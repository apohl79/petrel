/*
 * Copyright (c) 2016 Andreas Pohl
 * Licensed under MIT (see COPYING)
 *
 * Author: Andreas Pohl
 */

#include "lua_state_manager.h"
#include "asio_post.h"
#include "lib/library.h"
#include "lua_utils.h"
#include "make_unique.h"
#include "options.h"

#include <chrono>

namespace petrel {

thread_local std::unique_ptr<lua_state_manager::state_cache_type> lua_state_manager::m_state_cache_local;

std::vector<lib_reg> lua_state_manager::m_libs_builtin;
std::vector<lib_reg> lua_state_manager::m_libs;

// Initialize log members
init_log_static(lua_state_manager, log_priority::info);

lua_state_manager::lua_state_manager() {
    if (options::is_set("lua.devmode")) {
        log_notice("devmode activated. scripts will be reloaded on every request.");
        m_dev_mode = true;
    }
    lua_utils::load_script_dir(options::get_string("lua.root"), m_scripts);
    m_state_watcher = std::thread([this] {
        int buffer_size = options::get_int("lua.statebuffer", 5);
        while (!m_stop) {
            // relax
            std::this_thread::sleep_for(std::chrono::seconds(1));
            std::uint32_t to_create = 0;
            {
                std::lock_guard<std::mutex> lock(m_state_mtx);
                to_create = buffer_size - m_state_cache.size();
            }
            for (int i = to_create; i > 0; --i) {
                try {
                    auto Lex = create_state();
                    std::lock_guard<std::mutex> lock(m_state_mtx);
                    m_state_cache.push_back(Lex);
                } catch (std::exception& e) {
                    log_err(e.what());
                    break;
                }
            }
        }
    });
}

lua_state_manager::~lua_state_manager() {
    m_stop = true;
    m_state_watcher.join();
    for (auto Lex : m_state_cache) {
        destroy_state(Lex);
    }
    m_state_cache.clear();
    for (auto& lib : m_libs) {
        if (nullptr != lib.unload_func) {
            lib.unload_func();
        }
    }
    m_libs.clear();
}

void lua_state_manager::register_io_service(ba::io_service* iosvc) {
    m_iosvcs.push_back(iosvc);
    iosvc->post([] { m_state_cache_local = std::make_unique<state_cache_type>(); });
}

void lua_state_manager::unregister_io_service(ba::io_service* iosvc) {
    for (auto it = m_iosvcs.begin(); it != m_iosvcs.end(); it++) {
        if (*it == iosvc) {
            m_iosvcs.erase(it);
            break;
        }
    }
    io_service_post_wait(iosvc, [this] {
        for (auto Lex : *m_state_cache_local) {
            destroy_state(Lex);
        }
        m_state_cache_local.reset();
    });
}

int lua_state_manager::register_lib_builtin(const std::string& name, lua_CFunction open_func, lua_CFunction init_func,
                                            lib_load_func_type load_func, lib_load_func_type unload_func) {
    m_libs_builtin.push_back(lib_reg(name, open_func, init_func, unload_func));
    if (nullptr != load_func) {
        load_func();
    }
    return 0;
}

int lua_state_manager::register_lib(const std::string& name, lua_CFunction open_func, lua_CFunction init_func,
                                    lib_load_func_type load_func, lib_load_func_type unload_func) {
    m_libs.push_back(lib_reg(name, open_func, init_func, unload_func));
    if (nullptr != load_func) {
        load_func();
    }
    return 0;
}

void lua_state_manager::load_libs(lua_State* L) {
    for (auto& lib : m_libs_builtin) {
        lua_utils::lua_requiref(L, lib.name.c_str(), lib.open_func, 1);
        lib.init_func(L);
    }
    for (auto& lib : m_libs) {
        lua_utils::lua_requiref(L, lib.name.c_str(), lib.open_func, 1);
        lib.init_func(L);
    }
}

void lua_state_manager::print_registered_libs() {
    log_info("registered libraries");
    for (auto& lib : m_libs_builtin) {
        log_info("  " << lib.name << " (builtin)");
    }
    for (auto& lib : m_libs) {
        log_info("  " << lib.name);
    }
}

lua_state_ex lua_state_manager::create_state() {
    lua_state_ex Lex;
    Lex.L = luaL_newstate();
    if (unlikely(nullptr == Lex.L)) {
        throw std::runtime_error("luaL_newstate failed");
    }

    // load lua libs
    lua_utils::lua_requiref(Lex.L, "base", luaopen_base, 1);
    lua_utils::lua_requiref(Lex.L, "debug", luaopen_debug, 1);
    lua_utils::lua_requiref(Lex.L, "math", luaopen_math, 1);
    lua_utils::lua_requiref(Lex.L, "string", luaopen_string, 1);
    lua_utils::lua_requiref(Lex.L, "table", luaopen_table, 1);
    lua_utils::lua_requiref(Lex.L, "package", luaopen_package, 1);

    // Install our own print function
    lua_register(Lex.L, "print", lua_utils::print);

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
    Lex.code_version = 0;  // m_code_version;
    lua_utils::load_code_from_scripts(Lex.L, m_scripts);
    {
        std::lock_guard<std::mutex> lock(m_code_mtx);
        lua_utils::load_code_from_strings(Lex.L, m_code);
    }

    // Install traceback
    lua_pushcfunction(Lex.L, lua_utils::traceback);
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

void lua_state_manager::destroy_state(lua_state_ex L) {
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

lua_state_ex lua_state_manager::get_state() {
    lua_state_ex Lex;
    if (likely(nullptr != m_state_cache_local && !m_state_cache_local->empty())) {
        Lex = m_state_cache_local->back();
        m_state_cache_local->pop_back();
    } else {
        std::lock_guard<std::mutex> lock(m_state_mtx);
        if (likely(!m_state_cache.empty())) {
            Lex = m_state_cache.back();
            m_state_cache.pop_back();
        }
    }
    if (nullptr == Lex.L) {
        log_warn("creating a lua state in the request handler, you should increase the lua.statebuffer value!");
        Lex = create_state();
    } else {
        if (unlikely(m_dev_mode)) {
            lua_utils::load_code_from_scripts(Lex.L, m_scripts);
        }
    }
    return Lex;
}

void lua_state_manager::free_state(lua_state_ex Lex) {
    if (likely(nullptr != Lex.ctx->p_objects)) {
        for (auto* obj : *Lex.ctx->p_objects) {
            delete obj;
        }
        Lex.ctx->p_objects->clear();
    }
    if (likely(nullptr != m_state_cache_local)) {
        m_state_cache_local->push_back(Lex);
    } else {
        std::lock_guard<std::mutex> lock(m_state_mtx);
        m_state_cache.push_back(Lex);
    }
}

void lua_state_manager::add_lua_code(const std::string& code) {
    std::lock_guard<std::mutex> lock(m_code_mtx);
    m_code.push_back(code);
}

void lua_state_manager::clear_code() {
    std::lock_guard<std::mutex> lock(m_code_mtx);
    m_code.clear();
}

}  // petrel
