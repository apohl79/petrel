/*
 * Copyright (c) 2016 Andreas Pohl
 * Licensed under MIT (see COPYING)
 *
 * Author: Andreas Pohl
 */

#ifndef LUA_ENGINE_H
#define LUA_ENGINE_H

#include <atomic>
#include <initializer_list>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <boost/core/noncopyable.hpp>

#include <lua.hpp>

#include "log.h"
#include "lua_state_manager.h"
#include "metrics/counter.h"
#include "request.h"

namespace petrel {

class server;

class lua_engine : boost::noncopyable {
  public:
    decl_log_static();

    /// Ctor
    lua_engine() {}

    /// Call the bootstrap function in lua to setup the server
    void bootstrap(server& srv);

    /// Handle an incoming http request
    void handle_request(const std::string& func, request::pointer req);

    /// Return the state manager
    lua_state_manager& state_manager() { return m_state_mgr; }

  private:
    lua_state_manager m_state_mgr;

    /// Create a cookie table on the given lua state
    void push_cookies(lua_State* L, const std::string& cookies);
    /// Create a lua table object out of the http request and push it to the stack
    void push_request(lua_State* L, const request::pointer req);
    /// Create an empty response lua table that has all required fields and some defaults
    void push_response(lua_State* L);
};

}  // petrel

#endif  // LUA_ENGINE_H
