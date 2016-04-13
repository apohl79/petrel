/*
 * Copyright (c) 2016 Andreas Pohl
 * Licensed under MIT (see COPYING)
 *
 * Author: Andreas Pohl
 */

#include "lua_engine.h"
#include "lib/library.h"
#include "lua_utils.h"
#include "server.h"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <iostream>
#include <sstream>

#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>

#include <boost/utility/string_ref.hpp>

namespace petrel {

// Initialize log members
init_log_static(lua_engine, log_priority::info);

/// Log an error, dump the stack and throw a runtime_error
inline void log_throw(lua_State* L, const std::string& path, std::initializer_list<boost::string_ref> msg) {
    std::ostringstream os;
    set_log_tag(get_log_tag_static(lua_engine));
    set_log_priority(get_log_priority_static(lua_engine));
    log_default_noln("request " << path << " failed: ");
    for (auto& s : msg) {
        log_plain_noln(s << " ");
        os << s << " ";
    }
    if (nullptr != L) {
        log_plain("(enable log.level 7 to see the LUA stack)");
    } else {
        log_plain("");
    }
    if (nullptr != L) {
        lua_utils::dump_stack(L);
    }
    throw std::runtime_error(os.str());
}

void lua_engine::bootstrap(server& srv) {
    log_info("running bootstrap");
    auto Lex = m_state_mgr.create_state();
    auto* L = Lex.L;
    // As we have no io_service at this time, we create one. Async code will not work yet.
    boost::asio::io_service iosvc;
    Lex.ctx->p_server = &srv;
    Lex.ctx->p_io_service = &iosvc;
    // Call bootstrap
    lua_getglobal(L, "bootstrap");
    if (!lua_isfunction(L, -1)) {
        throw std::runtime_error("No bootstrap function defined");
    }
    if (lua_pcall(L, 0, 0, Lex.traceback_idx)) {
        throw std::runtime_error(std::string("bootstrap call failed: ") + std::string(lua_tostring(L, -1)));
    }
    m_state_mgr.destroy_state(Lex);
    m_state_mgr.print_registered_libs();
}

void lua_engine::handle_request(const std::string& func, request::pointer req) {
    auto Lex = m_state_mgr.get_state();
    auto* L = Lex.L;
    Lex.ctx->p_server = &req->get_server();
    Lex.ctx->p_io_service = &req->get_io_service();
    // Call func
    lua_getglobal(L, func.c_str());
    if (!lua_isfunction(L, -1)) {
        log_throw(L, req->path(), {"handler function", func, "not defined"});
    }
    push_request(L, req);
    push_response(L);
    if (lua_pcall(L, 2, 1, Lex.traceback_idx)) {
        log_throw(nullptr, req->path(), {"lua_pcall failed:", lua_tostring(L, -1)});
    }
    // Get the response
    if (!lua_istable(L, -1)) {
        log_throw(L, req->path(), {"repsonse is no table"});
    }
    lua_getfield(L, -1, "status");
    if (!lua_isnumber(L, -1)) {
        log_throw(L, req->path(), {"response.status is no number"});
    }
    int status = lua_tointeger(L, -1);
    lua_pop(L, 1);
    // Iterate through response headers
    lua_getfield(L, -1, "headers");
    if (!lua_istable(L, -1)) {
        log_throw(L, req->path(), {"repsonse.headers is no table"});
    }
    int hdr_i = lua_gettop(L);
    lua_pushnil(L);
    while (lua_next(L, hdr_i) != 0) {
        if (lua_isstring(L, -1) && lua_isstring(L, -2)) {
            const char* name = lua_tostring(L, -2);
            const char* val = lua_tostring(L, -1);
            req->add_header(name, val);
        }
        lua_pop(L, 1);
    }
    lua_pop(L, 1);
    // Get the content
    lua_getfield(L, -1, "content");
    if (!lua_isstring(L, -1)) {
        log_throw(L, req->path(), {"response.content is no string"});
    }
    std::size_t content_len;
    auto* content_ptr = lua_tolstring(L, -1, &content_len);
    req->send_response(status, boost::string_ref(content_ptr, content_len));
    // Clean up (remove return value and the traceback)
    lua_pop(L, 2);
    m_state_mgr.free_state(Lex);
}

void lua_engine::push_cookies(lua_State* L, const std::string& cookies) {
    lua_newtable(L);
    using namespace boost::algorithm;
    using it_type = boost::iterator_range<std::string::const_iterator>;
    std::vector<it_type> vec;
    // split cookie string by ';'
    for (auto&& part : iter_split(vec, cookies, token_finder(is_any_of(";")))) {
        auto* str = &part.front();
        auto len = part.size();
        // find the '=' char
        auto it = std::find(part.begin(), part.end(), '=');
        if (it != part.end() && it != part.begin()) {
            auto* eq = &(*it);
            auto* name = str;
            auto* val = eq + 1;
            // skip leading spaces
            while (*name == ' ' && name < eq) {
                ++name;
            }
            std::size_t name_len = eq - name;
            std::size_t val_len = len - name_len - 1;
            if (name_len > 0) {
                lua_pushlstring(L, name, name_len);
                lua_pushlstring(L, val, val_len);
                lua_rawset(L, -3);
            }
        }
    }
}

void lua_engine::push_request(lua_State* L, request::pointer req) {
    lua_createtable(L, 0, 9);
    lua_pushinteger(L, std::time(nullptr));
    lua_setfield(L, -2, "timestamp");
    lua_pushstring(L, req->method_string().c_str());
    lua_setfield(L, -2, "method");
    lua_pushstring(L, req->proto().c_str());
    lua_setfield(L, -2, "proto");
    lua_pushstring(L, req->host().c_str());
    lua_setfield(L, -2, "host");
    lua_pushstring(L, req->path().c_str());
    lua_setfield(L, -2, "path");
    lua_createtable(L, 0, req->headers_size());
    std::for_each(req->headers_begin(), req->headers_end(), [L](const request::header_type h) {
        lua_pushstring(L, h.second.c_str());
        lua_setfield(L, -2, h.first.c_str());
    });
    lua_setfield(L, -2, "headers");
    auto addr = req->remote_endpoint().address();
    lua_pushstring(L, addr.to_string().c_str());
    lua_setfield(L, -2, "remote_addr_str");
    lua_pushinteger(L, addr.is_v4() ? 4 : 6);
    lua_setfield(L, -2, "remote_addr_ip_ver");
    if (req->header_exists("cookie")) {
        push_cookies(L, req->header("cookie"));
        lua_setfield(L, -2, "cookies");
    }
    if (req->method() == request::http_method::POST) {
        auto content = req->content();
        lua_pushlstring(L, content.data(), content.size());
        lua_setfield(L, -2, "content");
    }
}

void lua_engine::push_response(lua_State* L) {
    lua_createtable(L, 0, 3);
    lua_pushinteger(L, 200);
    lua_setfield(L, -2, "status");
    lua_pushstring(L, "");
    lua_setfield(L, -2, "content");
    lua_createtable(L, 0, 0);
    lua_setfield(L, -2, "headers");
}

}  // petrel
