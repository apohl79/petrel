/*
 * Copyright (c) 2016 Andreas Pohl
 * Licensed under MIT (see COPYING)
 *
 * Author: Andreas Pohl
 */

#ifndef LUA_ENGINE_H
#define LUA_ENGINE_H

#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <initializer_list>

#include <boost/core/noncopyable.hpp>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "log.h"
#include "request.h"
#include "metrics/counter.h"

namespace petrel {

using lib_load_func_type = std::function<void()>;

/// Lib registration structure
struct lib_reg {
    lib_reg(const std::string& n, lua_CFunction o, lua_CFunction i, lib_load_func_type u)
        : name(n), open_func(o), init_func(i), unload_func(u) {}
    std::string name;
    lua_CFunction open_func;
    lua_CFunction init_func;
    lib_load_func_type unload_func;
};

namespace lib {
struct lib_context;
}

/// Keep a handle to the lua states, the traceback func index and the library context
struct lua_state_ex {
    lua_State* L = nullptr;
    int traceback_idx = 0;
    lib::lib_context* ctx = nullptr;
    std::uint_fast64_t code_version = 0;
};

class server;

class lua_engine : boost::noncopyable {
  public:
    lua_engine();
    ~lua_engine();

    // Add static log members
    decl_log_static();

    /// Start a thread that watches the available lua states. It creates new states or updates the available states with
    /// new lua code.
    void start(server& srv);
    /// Stop the lua engine.
    void stop();
    /// Stop the lua engine.
    void join();
    /// Assign a list of lua scripts. Running lua states get updated.
    void set_lua_scripts(std::vector<std::string> scripts);
    /// Reload lua scripts and the code buffer in all lua states
    void reload_lua_code();
    /// Add lua code from a string
    void add_lua_code_to_buffer(const std::string& code);
    /// Reset the code buffer
    void clear_code_buffer();

    /// Load a lua script into the code buffer
    static void load_code_from_file(lua_State* L, const std::string& script);
    /// Load lua code from the string into the given lua state
    static void load_code_from_string(lua_State* L, const std::string& code);

    /// Call the bootstrap function in lua to setup the server
    void bootstrap(server& srv);
    /// Handle an incoming http request
    void handle_request(const std::string& func, request::pointer req);

    /// Find scripts in a directory and it's sub directories
    static void load_script_dir(const std::string& dir, std::vector<std::string>& vec);
    /// Library helpers
    static int register_lib(const std::string& name, lua_CFunction open_func, lua_CFunction init_func,
                            lib_load_func_type load_func, lib_load_func_type unload_func);
    /// Print out all registered libs
    static void print_registered_libs();

    /// Create a new lua state
    lua_state_ex create_lua_state();
    /// Destroy a lua state
    void destroy_lua_state(lua_state_ex L);

    /// Dump the stack of a lua state
    static void dump_stack(lua_State* L);
    /// Print a lua type at the given stack index
    static void print_type(lua_State* L, int i, log_priority prio, bool show_type_name = true,
                           bool show_table_content = true, int max_table_depth = -1);
    /// Print a lua type at the given stack index (simple version)
    static void print_type_simple(lua_State* L, int i, log_priority prio, bool string_quotes = false,
                                  bool show_table = false, int max_table_depth = -1);
    /// Print a lua table at the given stack index
    static void print_table(lua_State* L, int i, log_priority prio, int level = 0, int max_level = -1);

  private:
    /// List of script filenames to be loaded
    std::vector<std::string> m_scripts;
    std::mutex m_scripts_mtx;

    /// The state buffer
    std::vector<lua_state_ex> m_states;
    std::mutex m_states_mtx;
    metrics::counter::pointer m_states_counter;
    metrics::counter::pointer m_statebuffer_counter;
    std::thread m_states_watcher;


    /// Termination flag
    std::atomic_bool m_stop{false};

    /// In dev mode the scripts get reloaded on every request
    bool m_dev_mode = false;

    /// Vector for all libs to be loaded
    static std::vector<lib_reg> m_libs;

    /// The code version gets incremented with each code reload and is used to verify if a lua state has the latest code
    /// version loaded.
    std::atomic_uint_fast64_t m_code_version{0};
    std::atomic_bool m_code_reloading{false};

    /// The code buffer contains all user lua code that gets loaded for request processing
    std::vector<std::string> m_code_buffer;
    std::mutex m_code_buffer_mtx;

    /// Fill the lua state buffer
    void fill_lua_state_buffer();
    /// Get a lua state from the pool.
    lua_state_ex get_lua_state();
    /// Return a state object to the pool.
    void free_lua_state(lua_state_ex L, bool reload_in_new_thread = true);

    /// Initialize libraries
    void load_libs(lua_State* L);
    /// Initialize library
    int load_lib(const lib_reg& lib, lua_State* L);
    /// Load lua code from the list of scripts into the given lua state
    void load_code_from_scripts(lua_State* L);
    /// Load lua code from the code buffer into the given lua state
    void load_code_from_buffer(lua_State* L);

    /// Create a cookie table on the given lua state
    void push_cookies(lua_State* L, const std::string& cookies);
    /// Create a lua table object out of the http request and push it to the stack
    void push_request(lua_State* L, const request::pointer req);
    /// Create an empty response lua table that has all required fields and some defaults
    void push_response(lua_State* L);

    /// Replacement for lua's print
    static int print(lua_State* L);
    /// Traceback function
    static int traceback(lua_State* L);
};

}  // petrel

#endif  // LUA_ENGINE_H
