/*
 * Copyright (c) 2016 Andreas Pohl
 * Licensed under MIT (see COPYING)
 *
 * Author: Andreas Pohl
 */

#ifndef LUA_STATE_MANAGER_H
#define LUA_STATE_MANAGER_H

#include <boost/asio.hpp>
#include <boost/core/noncopyable.hpp>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "log.h"
#include "lua_inc.h"

namespace petrel {

namespace lib {
struct lib_context;
}

/// Keep a handle to the lua state, the traceback func index and the library context
struct lua_state_ex {
    lua_State* L = nullptr;
    int traceback_idx = 0;
    lib::lib_context* ctx = nullptr;
    std::uint_fast64_t code_version = 0;
};

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

namespace ba = boost::asio;

/// The lua_state_manager manages lua states by keeping states cached locally to a thread to avoid locking. It
/// creates states and sets up the lua environment for a state by loading the lua code and libraries etc. It will try to
/// avoid creating states when handling a request as much as possible.
class lua_state_manager : boost::noncopyable {
    decl_log_static();

  public:
    lua_state_manager();
    ~lua_state_manager();

    /// Get a lua state from the pool.
    ///
    /// @return A state object
    lua_state_ex get_state();

    /// Return a state object to the pool.
    ///
    /// @param L The state object
    void free_state(lua_state_ex L);

    /// Create a new lua state
    lua_state_ex create_state();

    /// Destroy a lua state
    void destroy_state(lua_state_ex L);

    /// Register an io service object. As we are running one io service per worker we can use post() to execute
    /// cache updates in the contect of each worker and maintain thread local caches that require no locks.
    ///
    /// @param iosvc The io service object to register
    void register_io_service(ba::io_service* iosvc);

    /// Unregister an io service object.
    ///
    /// @param iosvc The io service object to unregister
    void unregister_io_service(ba::io_service* iosvc);

    /// Register a builtin library
    ///
    /// @param name The library name
    /// @param open_func The open function will be called to open the library to load it into a state
    /// @param init_func The init function will be called to initialize the library after it has been opened
    /// @param load_func The load function will be called once after the library shared object has been loaded
    /// @param unload_func The unload function will be called before a shared object gets closed
    static int register_lib_builtin(const std::string& name, lua_CFunction open_func, lua_CFunction init_func,
                                    lib_load_func_type load_func, lib_load_func_type unload_func);

    /// Register a dynamic external library
    ///
    /// @param name The library name
    /// @param open_func The open function will be called to open the library to load it into a state
    /// @param init_func The init function will be called to initialize the library after it has been opened
    /// @param load_func The load function will be called once after the library shared object has been loaded
    /// @param unload_func The unload function will be called before a shared object gets closed
    static int register_lib(const std::string& name, lua_CFunction open_func, lua_CFunction init_func,
                            lib_load_func_type load_func, lib_load_func_type unload_func);

    /// Print out all registered libs
    static void print_registered_libs();

    /// Add lua code from a string
    void add_lua_code(const std::string& code);

    /// Reset the code buffer
    void clear_code();

  private:
    // The thread local state cache
    using state_cache_type = std::vector<lua_state_ex>;
    state_cache_type m_state_cache;
    std::mutex m_state_mtx;

    static thread_local std::unique_ptr<state_cache_type> m_state_cache_local;

    /// A thread will make sure the state cache has enough state objects available and precreates them.
    std::thread m_state_watcher;

    /// Termination flag
    std::atomic_bool m_stop{false};

    /// Registered io services
    std::vector<ba::io_service*> m_iosvcs;

    /// Vector for all builtin libs to be loaded
    static std::vector<lib_reg> m_libs_builtin;

    /// Vector for all external libs to be loaded
    static std::vector<lib_reg> m_libs;

    /// List of script filenames to be loaded
    std::vector<std::string> m_scripts;

    /// The code vector contains all user lua code that gets loaded on the fly
    std::vector<std::string> m_code;
    std::mutex m_code_mtx;

    /// In dev mode the scripts get reloaded on every request
    bool m_dev_mode = false;

    /// Initialize libraries
    static void load_libs(lua_State* L);
};

}  // petrel

#endif
