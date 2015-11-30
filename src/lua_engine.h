#ifndef LUA_ENGINE_H
#define LUA_ENGINE_H

#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <initializer_list>

#include <boost/core/noncopyable.hpp>
#include <boost/fiber/all.hpp>

#include <nghttp2/asio_http2_server.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "log.h"
#include "session.h"
#include "metrics/counter.h"

namespace petrel {

namespace http = boost::http;
namespace http2 = nghttp2::asio_http2;
namespace bf = boost::fibers;

/// Lib registration structure
struct lib_reg {
    lib_reg(const std::string& n, lua_CFunction o, lua_CFunction i) : name(n), open_func(o), init_func(i) {}
    std::string name;
    lua_CFunction open_func;
    lua_CFunction init_func;
};

namespace lib {
struct lib_context;
}

/// Keep a handle to the lua states, the traceback func index and the library context
struct lua_state_ex {
    lua_State* L = nullptr;
    int traceback_idx = 0;
    lib::lib_context* ctx = nullptr;
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
    /// Assign a list of lua scripts. Running lua states get updated.
    void set_lua_scripts(std::vector<std::string> scripts);
    /// Reload lua scripts in all lua states
    void reload_lua_scripts();
    /// Call the bootstrap function in lua to setup the server
    void bootstrap(server& srv);
    /// Handle an incoming http request
    void handle_http_request(const std::string& func, session::request_type::pointer req);
    /// Handle an incoming http2 request
    void handle_http_request(const std::string& func, const http2::server::request& req,
                             const http2::server::response& res, server& srv);

    /// Find scripts in a directory and it's sub directories
    static void load_script_dir(const std::string& dir, std::vector<std::string>& vec);
    /// Library helpers
    static int register_lib(const std::string& name, lua_CFunction open_func, lua_CFunction init_func);
    /// Print out all registered libs
    static void print_registered_libs();

    /// Dump the stack of a lua state
    static void dump_stack(lua_State* L);
    /// Print a lua type at the given stack index
    static void print_type(lua_State* L, int i, log_priority prio, bool show_type_name = true,
                          bool show_table_content = true);
    /// Print a lua type at the given stack index (simple version)
    static void print_type_simple(lua_State* L, int i, log_priority prio, bool string_quotes = false,
                                  bool show_table = false);
    /// Print a lua table at the given stack index
    static void print_table(lua_State* L, int i, log_priority prio);

  private:
    std::mutex m_scripts_mtx;
    std::vector<std::string> m_scripts;
    std::mutex m_states_mtx;
    std::vector<lua_state_ex> m_states;
    metrics::counter::pointer m_states_counter;
    metrics::counter::pointer m_statebuffer_counter;
    std::thread m_states_watcher;
    std::atomic_bool m_stop{false};
    bool m_dev_mode = false;

    /// Vector for all libs
    static std::vector<lib_reg> m_libs;

    /// Create a new lua state
    lua_state_ex create_lua_state();
    /// Fill the lua state buffer
    void fill_lua_state_buffer();
    /// Get a lua state from the pool.
    lua_state_ex get_lua_state();
    /// Return a state object to the pool.
    void free_lua_state(lua_state_ex L);

    /// Initialize libraries
    void load_libs(lua_State* L);
    /// Initialize library
    int load_lib(const lib_reg& lib, lua_State* L);
    /// Load a lua script into the current threads lua state
    void load_script(lua_State* L, const std::string& script);

    /// Create a lua table object out of the http request and push it to the stack
    void push_http_request(lua_State* L, const session::request_type::pointer req);
    /// Create a lua table object out of the http2 request and push it to the stack
    void push_http_request(lua_State* L, const http2::server::request& req, const std::string& path);
    /// Create an empty response lua table that has all required fields and some defaults
    void push_http_response(lua_State* L);

    /// Replacement for lua's print
    static int print(lua_State* L);
    /// Traceback function
    static int traceback(lua_State* L);
};

}  // petrel

#endif  // LUA_ENGINE_H
