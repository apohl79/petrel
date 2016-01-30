/*
 * Copyright (c) 2016 Andreas Pohl
 * Licensed under MIT (see COPYING)
 *
 * Author: Andreas Pohl
 */

#include "http_client.h"
#include "log.h"
#include "server.h"
#include "resolver_cache.h"

#include <string>
#include <vector>
#include <algorithm>
#include <cstring>
#include <cstdlib>

#include <boost/utility/string_ref.hpp>
#include <boost/fiber/all.hpp>
#include <boost/bind.hpp>
#include <boost/algorithm/string.hpp>
#include <petrel/fiber/yield.hpp>

namespace petrel {
namespace lib {

DECLARE_LIB_BEGIN(http_client);
ADD_LIB_METHOD(connect);
ADD_LIB_METHOD(disconnect);
ADD_LIB_METHOD(get);
ADD_LIB_METHOD(post);
ADD_LIB_METHOD(read_timeout);
ADD_LIB_METHOD(connect_timeout);
ADD_LIB_METHOD(disable_keepalive);
DECLARE_LIB_BUILTIN_END();

namespace bf = boost::fibers;
namespace bfa = bf::asio;
namespace balg = boost::algorithm;

void http_client::timeout_handler(const bs::error_code& ec) {
    if (ba::error::operation_aborted != ec) {
        m_sock.cancel();
    }
}

int http_client::connect(lua_State* L) {
    m_host = luaL_checkstring(L, 1);
    if (lua_isstring(L, 2)) {
        m_port = lua_tostring(L, 2);
    } else {
        m_port = "80";
    }
    using boost::asio::ip::tcp;
    // DNS lookup and connect
    try {
        auto& resolver = context().server().get_resolver_cache();
        auto ep_iter = resolver.async_resolve<resolver_cache::tcp>(io_service(), m_host, m_port, bfa::yield);
        ba::deadline_timer timer(io_service());
        timer.expires_from_now(m_connect_timeout);
        timer.async_wait(boost::bind(&http_client::timeout_handler, this, ba::placeholders::error));
        ba::async_connect(m_sock, ep_iter, bfa::yield);
        timer.cancel();
        m_connected = true;
    } catch (bs::system_error& e) {
        luaL_error(L, "connect failed: %s", e.what());
    }
    return 0;  // no results
}

int http_client::disconnect(lua_State*) {
    close();
    return 0;  // no results
}

int http_client::get(lua_State* L) {
    boost::string_ref path(luaL_checkstring(L, 1));
    if (!m_connected) {
        luaL_error(L, "not connected");
    }
    // Create request
    std::ostream request_strm(&m_req_buf);
    request_strm << "GET " << path << " HTTP/1.1\r\n";
    request_strm << "Host: " << m_host << "\r\n";
    request_strm << "Accept: */*\r\n";
    request_strm << "User-Agent: petrel/http_client\r\n";
    if (!m_keep_alive) {
        request_strm << "Connection: keep-alive\r\n";
    }
    request_strm << "\r\n";
    return send_recv(L);
}

int http_client::post(lua_State* L) {
    boost::string_ref path(luaL_checkstring(L, 1));
    const char* data = nullptr;
    std::size_t len = 0;
    if (lua_isstring(L, 2)) {
        data = lua_tolstring(L, 2, &len);
    }
    if (!m_connected) {
        luaL_error(L, "not connected");
    }
    // Create request
    std::ostream request_strm(&m_req_buf);
    request_strm << "POST " << path << " HTTP/1.1\r\n";
    request_strm << "Host: " << m_host << "\r\n";
    request_strm << "Accept: */*\r\n";
    request_strm << "User-Agent: petrel/http_client\r\n";
    if (!m_keep_alive) {
        request_strm << "Connection: keep-alive\r\n";
    }
    request_strm << "Content-Length: " << len << "\r\n\r\n";
    if (len > 0 && nullptr != data) {
        request_strm.write(data, len);
    }
    return send_recv(L);
}

int http_client::send_recv(lua_State* L) {
    try {
        // send query
        ba::async_write(m_sock, m_req_buf, bfa::yield);
        ba::deadline_timer timer(io_service());
        timer.expires_from_now(m_read_timeout);
        timer.async_wait(boost::bind(&http_client::timeout_handler, this, ba::placeholders::error));
        bs::error_code ec;
        ba::async_read_until(m_sock, m_res_buf, "\r\n\r\n", bfa::yield[ec]);
        timer.cancel();
        if (ec && ec != ba::error::eof) {
            luaL_error(L, "read failed: %s", ec.message().c_str());
        }
        std::istream response_strm(&m_res_buf);
        std::string line;
        // read status line
        std::getline(response_strm, line);
        if (!response_strm || line.find("HTTP/") != 0) {
            luaL_error(L, "invalid response");
        }
        auto pos = line.find(' ', 5);
        if (std::string::npos == pos) {
            luaL_error(L, "invalid response: no status");
        }
        int status = std::atoi(line.c_str() + pos + 1);
        if (status < 100 || status > 999) {
            luaL_error(L, "invalid response: invalid status %d");
        }
        // read headers
        std::string location;
        std::size_t content_len = 0;
        while (std::getline(response_strm, line) && line != "\r") {
            if (balg::istarts_with(line, "location: ")) {
                location = line.substr(10);
            } else if (balg::istarts_with(line, "content-length: ")) {
                content_len = std::atoi(line.c_str() + 16);
            }
        }
        // check for redirect
        if (status == 302) {
            if (location.empty()) {
                luaL_error(L, "invalid 302 redirect: no location header");
            }
            lua_pushinteger(L, status);
            lua_pushstring(L, location.c_str());
        } else {
            lua_pushinteger(L, status);
            if (content_len > 0) {
                auto to_read = content_len - m_res_buf.size();
                if (to_read > 0) {
                    // if the buffer does not contain the full content yet, receive the rest
                    timer.expires_from_now(m_read_timeout);
                    timer.async_wait(boost::bind(&http_client::timeout_handler, this, ba::placeholders::error));
                    ba::async_read(m_sock, m_res_buf, boost::asio::transfer_at_least(to_read), bfa::yield[ec]);
                    timer.cancel();
                    if (ec && ec != ba::error::eof) {
                        luaL_error(L, "read failed: %s", ec.message().c_str());
                    }
                }
                luaL_Buffer lbuf;
                luaL_buffinitsize(L, &lbuf, content_len);
                std::for_each(ba::buffers_begin(m_res_buf.data()), ba::buffers_end(m_res_buf.data()),
                              [&lbuf](const char c) { luaL_addchar(&lbuf, c); });
                luaL_pushresult(&lbuf);
            } else {
                lua_pushstring(L, "");
            }
        }
    } catch (bs::system_error& e) {
        luaL_error(L, e.what());
    }
    return 2;  // status code and data makes 2 results
}

int http_client::read_timeout(lua_State* L) {
    int t = luaL_checkinteger(L, 1);
    m_read_timeout = bp::millisec(t);
    return 0;
}

int http_client::connect_timeout(lua_State* L) {
    int t = luaL_checkinteger(L, 1);
    m_connect_timeout = bp::millisec(t);
    return 0;
}

int http_client::disable_keepalive(lua_State* L) {
    m_keep_alive = false;
    return 0;
}

}  // lib
}  // petrel
