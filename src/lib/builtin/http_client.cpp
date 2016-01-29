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
#include <cstring>
#include <cstdlib>

#include <boost/utility/string_ref.hpp>
#include <boost/fiber/all.hpp>
#include <petrel/fiber/yield.hpp>

namespace petrel {
namespace lib {

DECLARE_LIB_BEGIN(http_client);
ADD_LIB_METHOD(connect);
ADD_LIB_METHOD(disconnect);
ADD_LIB_METHOD(get);
DECLARE_LIB_BUILTIN_END();

namespace bs = boost::system;
namespace bf = boost::fibers;
namespace bfa = bf::asio;

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
        ba::async_connect(m_sock, ep_iter, bfa::yield);
        m_connected = true;
    } catch (bs::system_error& e) {
        luaL_error(L, "connect failed: %s", e.what());
    }
    return 0; // no results
}

int http_client::disconnect(lua_State*) {
    close();
    return 0; // no results
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
    request_strm << "Connection: close\r\n\r\n";

    try {
        // send query
        ba::async_write(m_sock, m_req_buf, bfa::yield);
        bs::error_code ec;
        std::size_t n = ba::async_read(m_sock, ba::buffer(m_res_buf, sizeof(m_res_buf) - 1), bfa::yield[ec]);
        if (ec && ec != ba::error::eof) {
            luaL_error(L, "read failed: %s", ec.message().c_str());
        }
        m_res_buf[n] = 0;
        // read status line
        if (std::strncmp(m_res_buf, "HTTP/", 5)) {
            luaL_error(L, "invalid response");
        }
        const char* pos = std::strchr(m_res_buf + 5, ' ');
        if (nullptr == pos) {
            close();
            luaL_error(L, "invalid response: no status");
        }
        int status = std::atoi(pos + 1);
        if (status < 100 || status > 999) {
            luaL_error(L, "invalid response: invalid status %d");
        }
        pos = std::strchr(pos + 4, '\n');
        if (nullptr == pos) {
            luaL_error(L, "invalid response: newline expected");
        }
        pos++;
        // read headers
        std::string location;
        while (*pos != 0) {
            // end of headers check
            if (*pos == '\r' || *pos == '\n') {
                pos++;
                if (*pos == '\n') {
                    pos++;
                }
                break;
            }
            // look for the next line break
            const char* eol = std::strchr(pos, '\n');
            if (nullptr == eol) {
                luaL_error(L, "invalid header: newline expected");
            }
            // look for the location header
            if (status == 302 && location.empty() && !strncmp(pos, "Location: ", 10)) {
                const char* loc_start = pos + 10;
                char* loc_end = const_cast<char*>(eol);
                while (*(loc_end - 1) == '\r' || *(loc_end - 1) == '\n') {
                    loc_end--;
                }
                *loc_end = 0;
                location = loc_start;
            }
            pos = eol + 1;
        };
        // check for redirect
        if (status == 302) {
            if (location.empty()) {
                luaL_error(L, "invalid 302 redirect: no location header");
            }
            lua_pushinteger(L, status);
            lua_pushstring(L, location.c_str());
        } else {
            lua_pushinteger(L, status);
            lua_pushstring(L, pos);
        }
    } catch (bs::system_error& e) {
        luaL_error(L, e.what());
    }
    return 2; // status code and data makes 2 results
}

}  // lib
}  // petrel
