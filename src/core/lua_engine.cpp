/*
 * Copyright (c) 2016 Andreas Pohl
 * Licensed under MIT (see COPYING)
 *
 * Author: Andreas Pohl
 */

#include "lua_engine.h"
#include "lib/library.h"
#include "metrics/registry.h"
#include "options.h"
#include "server.h"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <iostream>
#include <sstream>

#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <boost/utility/string_ref.hpp>

namespace petrel {

// Initialize log members
init_log_static(lua_engine, log_priority::info);

std::vector<lib_reg> lua_engine::m_libs;

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
        lua_engine::dump_stack(L);
    }
    throw std::runtime_error(os.str());
}

lua_engine::lua_engine() {
    if (options::opts.count("lua.devmode")) {
        log_notice("devmode activated. scripts will be reloaded on every request.");
        m_dev_mode = true;
    }
}

lua_engine::~lua_engine() {
    stop();
    join();
    for (auto Lex : m_states) {
        destroy_lua_state(Lex);
    }
    m_states.clear();
    for (auto& lib : m_libs) {
        if (nullptr != lib.unload_func) {
            lib.unload_func();
        }
    }
    m_libs.clear();
}

void lua_engine::print_registered_libs() {
    log_info("registered libraries");
    for (auto& lib : lua_engine::m_libs) {
        log_info("  " << lib.name);
    }
}

void lua_engine::start(server& srv) {
    print_registered_libs();
    log_info("initializing state buffer");
    m_statebuffer_counter = srv.get_metrics_registry().register_metric<metrics::counter>("lua_statebuffer");
    m_states_counter = srv.get_metrics_registry().register_metric<metrics::counter>("lua_states_created");
    fill_lua_state_buffer();
    log_debug("created " << m_states_counter->get() << " states");
    m_states_watcher = std::thread([this] {
        while (!m_stop) {
            fill_lua_state_buffer();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    });
}

void lua_engine::stop() { m_stop = true; }

void lua_engine::join() {
    if (m_states_watcher.joinable()) {
        m_states_watcher.join();
    }
}

void lua_engine::fill_lua_state_buffer() {
    int buffer_size = options::opts["lua.statebuffer"].as<int>();
    int to_create = 0;
    {
        std::lock_guard<std::mutex> lock(m_states_mtx);
        to_create = buffer_size - m_states.size();
    }
    for (int i = to_create; i > 0; --i) {
        auto Lex = create_lua_state();
        m_states_counter->increment();
        m_statebuffer_counter->increment();
        std::lock_guard<std::mutex> lock(m_states_mtx);
        m_states.push_back(Lex);
    }
}

lua_state_ex lua_engine::create_lua_state() {
    lua_state_ex Lex;
    Lex.L = luaL_newstate();
    if (nullptr == Lex.L) {
        throw std::runtime_error("luaL_newstate failed");
    }
    // load lua libs
    luaL_requiref(Lex.L, "base", luaopen_base, 1);
    luaL_requiref(Lex.L, "debug", luaopen_debug, 1);
    luaL_requiref(Lex.L, "math", luaopen_math, 1);
    luaL_requiref(Lex.L, "string", luaopen_string, 1);
    luaL_requiref(Lex.L, "table", luaopen_table, 1);
    luaL_requiref(Lex.L, "package", luaopen_package, 1);
    // Install our own print function
    lua_register(Lex.L, "print", print);
    // Define global log priorities
    lua_pushinteger(Lex.L, log::to_int(log_priority::emerg));
    lua_setglobal(Lex.L, "LOG_EMERG");
    lua_pushinteger(Lex.L, log::to_int(log_priority::alert));
    lua_setglobal(Lex.L, "LOG_ALERT");
    lua_pushinteger(Lex.L, log::to_int(log_priority::crit));
    lua_setglobal(Lex.L, "LOG_CRIT");
    lua_pushinteger(Lex.L, log::to_int(log_priority::err));
    lua_setglobal(Lex.L, "LOG_ERR");
    lua_pushinteger(Lex.L, log::to_int(log_priority::warn));
    lua_setglobal(Lex.L, "LOG_WARN");
    lua_pushinteger(Lex.L, log::to_int(log_priority::notice));
    lua_setglobal(Lex.L, "LOG_NOTICE");
    lua_pushinteger(Lex.L, log::to_int(log_priority::info));
    lua_setglobal(Lex.L, "LOG_INFO");
    lua_pushinteger(Lex.L, log::to_int(log_priority::debug));
    lua_setglobal(Lex.L, "LOG_DEBUG");
    // Initialize libs
    load_libs(Lex.L);

    // Load the user code
    Lex.code_version = m_code_version;
    load_code_from_scripts(Lex.L);
    load_code_from_buffer(Lex.L);

    // Install traceback
    lua_pushcfunction(Lex.L, traceback);
    Lex.traceback_idx = lua_gettop(Lex.L);

    // Create library context object
    Lex.ctx = reinterpret_cast<lib::lib_context*>(lua_newuserdata(Lex.L, sizeof(lib::lib_context)));
    Lex.ctx->p_server = nullptr;
    Lex.ctx->p_io_service = nullptr;
    Lex.ctx->p_objects = new std::vector<lib::library*>;
    // Add the context to the global env
    lua_setglobal(Lex.L, "petrel_context");

    return Lex;
}

void lua_engine::destroy_lua_state(lua_state_ex L) {
    if (nullptr != L.L) {
        if (nullptr != L.ctx->p_objects) {
            for (auto* obj : *L.ctx->p_objects) {
                delete obj;
            }
            L.ctx->p_objects->clear();
            delete L.ctx->p_objects;
        }
        lua_close(L.L);
    }
}

lua_state_ex lua_engine::get_lua_state() {
    lua_state_ex Lex;
    {
        std::lock_guard<std::mutex> lock(m_states_mtx);
        if (!m_states.empty()) {
            Lex = m_states.back();
            m_states.pop_back();
        }
    }
    if (nullptr == Lex.L) {
        log_warn("creating a lua state in the request handler, you should increase the lua.statebuffer value!");
        Lex = create_lua_state();
        m_states_counter->increment();
    } else {
        m_statebuffer_counter->decrement();
        if (m_dev_mode) {
            try {
                load_code_from_scripts(Lex.L);
                load_code_from_buffer(Lex.L);
            } catch (std::runtime_error& e) {
                log_err("failed to reload scripts: " << e.what());
            }
        }
    }
    return Lex;
}

void lua_engine::free_lua_state(lua_state_ex L, bool reload_in_new_thread) {
    if (nullptr != L.ctx->p_objects) {
        for (auto* obj : *L.ctx->p_objects) {
            delete obj;
        }
        L.ctx->p_objects->clear();
    }
    if (L.code_version != m_code_version) {
        auto f = [this, L]() mutable {
            L.code_version = m_code_version;
            load_code_from_scripts(L.L);
            load_code_from_buffer(L.L);
            free_lua_state(L, false /* we are in a separate thread already */);
        };
        if (reload_in_new_thread) {
            std::thread(f).detach();
        } else {
            f();
        }
    } else {
        m_statebuffer_counter->increment();
        std::lock_guard<std::mutex> lock(m_states_mtx);
        m_states.push_back(L);
    }
}

void lua_engine::load_libs(lua_State* L) {
    for (auto& lib : lua_engine::m_libs) {
        luaL_requiref(L, lib.name.c_str(), lib.open_func, 1);
        lib.init_func(L);
    }
}

void lua_engine::load_script_dir(const std::string& dir, std::vector<std::string>& vec) {
    using namespace boost::filesystem;
    log_info("loading lua directory: " << dir);
    path p(dir);
    std::vector<std::string> subdirs;
    try {
        for (directory_entry& entry : directory_iterator(p)) {
            const std::string& s = entry.path().string();
            if (is_directory(entry.status())) {
                subdirs.push_back(s);
            } else {
                if (s.substr(s.size() - 4) == ".lua") {
                    log_info("  found " << s);
                    vec.push_back(s);
                }
            }
        }
    } catch (filesystem_error& e) {
        throw std::runtime_error(e.what());
    }
    for (auto& subdir : subdirs) {
        load_script_dir(subdir, vec);
    }
}

void lua_engine::set_lua_scripts(std::vector<std::string> scripts) {
    std::lock_guard<std::mutex> lock(m_scripts_mtx);
    m_scripts = std::move(scripts);
}

int lua_engine::register_lib(const std::string& name, lua_CFunction open_func, lua_CFunction init_func,
                             lib_load_func_type load_func, lib_load_func_type unload_func) {
    m_libs.push_back(lib_reg(name, open_func, init_func, unload_func));
    if (nullptr != load_func) {
        load_func();
    }
    return 0;
}

void lua_engine::add_lua_code_to_buffer(const std::string& code) {
    std::lock_guard<std::mutex> lock(m_code_buffer_mtx);
    m_code_buffer.push_back(code);
}

void lua_engine::clear_code_buffer() {
    std::lock_guard<std::mutex> lock(m_code_buffer_mtx);
    m_code_buffer.clear();
}

void lua_engine::load_code_from_scripts(lua_State* L) {
    std::lock_guard<std::mutex> lock(m_scripts_mtx);
    for (auto& s : m_scripts) {
        load_code_from_file(L, s);
    }
}

void lua_engine::load_code_from_buffer(lua_State* L) {
    std::lock_guard<std::mutex> lock(m_code_buffer_mtx);
    for (auto& c : m_code_buffer) {
        load_code_from_string(L, c);
    }
}

void lua_engine::load_code_from_file(lua_State* L, const std::string& script) {
    if (luaL_dofile(L, script.c_str())) {
        throw std::runtime_error(lua_tostring(L, -1));
    }
}

void lua_engine::load_code_from_string(lua_State* L, const std::string& code) {
    if (luaL_dostring(L, code.c_str())) {
        throw std::runtime_error(lua_tostring(L, -1));
    }
}

void lua_engine::reload_lua_code() {
    if (!m_code_reloading.exchange(true)) {
        log_info("triggering code reload");
        std::thread([this] {
            m_code_version++;
            std::size_t count = 0;
            while (true) {
                lua_state_ex Lex;
                {
                    std::lock_guard<std::mutex> lock(m_states_mtx);
                    if (!m_states.empty()) {
                        Lex = m_states.front();
                        m_states.erase(m_states.begin());
                    } else {
                        break;
                    }
                }
                if (Lex.code_version == m_code_version) {
                    // found a state with the new version, we can stop
                    break;
                }
                Lex.code_version = m_code_version;
                load_code_from_scripts(Lex.L);
                load_code_from_buffer(Lex.L);
                free_lua_state(Lex);
                count++;
                // keep the impact to processing requests low
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
            log_info("code reload complete: " << count << " states updated");
            m_code_reloading = false;
        }).detach();
    } else {
        log_warn("not triggering code reload as a reload is in progess");
    }
}

void lua_engine::print_type(lua_State* L, int i, log_priority prio, bool show_type_name, bool show_table_content,
                            int max_table_depth) {
    set_log_priority(prio);
    if (lua_type(L, i) == LUA_TSTRING || lua_type(L, i) == LUA_TNUMBER) {
        print_type_simple(L, i, prio);
    } else if (lua_iscfunction(L, i) || lua_islightuserdata(L, i) || lua_isuserdata(L, i) || lua_isthread(L, i)) {
        if (show_type_name) {
            log_plain_noln(lua_typename(L, lua_type(L, i)) << ": ");
        }
        log_plain_noln(lua_topointer(L, i));
    } else if (lua_istable(L, i)) {
        if (show_type_name) {
            log_plain_noln(lua_typename(L, lua_type(L, i)) << ": ");
        }
        log_plain_noln(lua_topointer(L, i));
        if (show_table_content) {
            log_plain_noln(": ");
            print_table(L, i, prio, 0, max_table_depth);
        }
    } else {
        if (show_type_name) {
            log_plain_noln(lua_typename(L, lua_type(L, i)) << ": ");
        }
        log_plain_noln(lua_topointer(L, i));
    }
}

void lua_engine::print_type_simple(lua_State* L, int i, log_priority prio, bool string_quotes, bool show_table,
                                   int max_table_depth) {
    set_log_priority(prio);
    if (lua_type(L, i) == LUA_TSTRING) {
        if (string_quotes) {
            log_plain_noln("\"");
        }
        log_plain_noln(lua_tostring(L, i));
        if (string_quotes) {
            log_plain_noln("\"");
        }
    } else if (lua_type(L, i) == LUA_TNUMBER) {
        if (lua_isinteger(L, i)) {
            log_plain_noln(lua_tointeger(L, i));
        } else {
            log_plain_noln(lua_tonumber(L, i));
        }
    } else if (lua_type(L, i) == LUA_TTABLE) {
        if (show_table) {
            print_table(L, i, prio, 0, max_table_depth);
        } else {
            log_plain_noln("table...");
        }
    } else {
        log_plain_noln(lua_typename(L, lua_type(L, i)));
    }
}

void lua_engine::print_table(lua_State* L, int i, log_priority prio, int level, int max_level) {
    set_log_priority(prio);
    log_plain_noln(log_color::yellow << "{" << log_color::reset);
    int t = i < 0 ? lua_gettop(L) + i + 1 : i;
    bool first = true;
    lua_pushnil(L);
    while (lua_next(L, t) != 0) {
        if (first) {
            first = false;
        } else {
            log_plain_noln(", ");
        }
        log_plain_noln(log_color::white << "{" << log_color::reset);
        print_type_simple(L, -2, prio, true);
        log_plain_noln(", ");
        if (lua_istable(L, -1) && (max_level < 0 || level < max_level)) {
            print_table(L, -1, prio, level + 1, max_level);
        } else {
            print_type_simple(L, -1, prio, true, false);
        }
        log_plain_noln(log_color::white << "}" << log_color::reset);
        lua_pop(L, 1);
    }
    log_plain_noln(log_color::yellow << "}" << log_color::reset);
}

int lua_engine::print(lua_State* L) {
    set_log_tag("lua");
    set_log_priority(log_priority::info);
    int n = lua_gettop(L);
    if (n > 0) {
        int i = 1;
        if (lua_isinteger(L, i)) {
            int prio = lua_tointeger(L, i);
            if (log::is_log_priority(prio)) {
                update_log_priority(log::to_priority(prio));
                i++;
            }
        }
        log_default_noln("");
        for (; i <= n; i++) {
            print_type(L, i, get_log_priority(), true, true);
            if (i < n) {
                log_plain_noln("\t");
            }
        }
    } else {
        log_default_noln("");
    }
    log_plain("");
    return 0;
}

void lua_engine::dump_stack(lua_State* L) {
    set_log_priority(log_priority::debug);
    log_debug("STACK START " << L);
    int s = lua_gettop(L);
    bool nil_at_top = LUA_TNIL == lua_type(L, s);
    for (int i = s; i > 0; i--) {
        log_debug_noln("  " << log_color::white << i << ": " << log_color::reset << lua_typename(L, lua_type(L, i))
                            << ": ");
        print_type(L, i, get_log_priority(), false, true, 1);
        log_plain("");
        if (i == s && nil_at_top) {
            lua_remove(L, s);
        }
    }
    log_debug("STACK END " << L);
    if (nil_at_top) {
        lua_pushnil(L);
    }
}

int lua_engine::traceback(lua_State* L) {
    const char* msg = lua_tostring(L, 1);
    if (msg) {
        luaL_traceback(L, L, msg, 1);
    } else if (!lua_isnoneornil(L, 1)) {
        if (!luaL_callmeta(L, 1, "__tostring")) {
            lua_pushliteral(L, "(no error message)");
        }
    }
    return 1;
}

void lua_engine::bootstrap(server& srv) {
    log_info("running bootstrap");
    auto Lex = create_lua_state();
    auto* L = Lex.L;
    // As we have no io_service at this time, we create one. Async code will not work yet.
    boost::asio::io_service iosvc;
    Lex.ctx->p_server = &srv;
    Lex.ctx->p_io_service = &iosvc;
    // Call prepare
    lua_getglobal(L, "bootstrap");
    if (!lua_isfunction(L, -1)) {
        throw std::runtime_error("No bootstrap function defined");
    }
    if (lua_pcall(L, 0, 0, Lex.traceback_idx)) {
        throw std::runtime_error(std::string("bootstrap call failed: ") + std::string(lua_tostring(L, -1)));
    }
    destroy_lua_state(Lex);
}

void lua_engine::handle_request(const std::string& func, request::pointer req) {
    auto Lex = get_lua_state();
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
    free_lua_state(Lex);
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
    lua_pushstring(L, req->method().c_str());
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
    if (req->method()[0] == 'P') {  // we allow only GET and POST, so checking the first char is enough
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
