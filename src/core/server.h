/*
 * Copyright (c) 2016 Andreas Pohl
 * Licensed under MIT (see COPYING)
 *
 * Author: Andreas Pohl
 */

#ifndef SERVER_H
#define SERVER_H

#include <boost/core/noncopyable.hpp>

namespace petrel {

class server_impl;
class lua_engine;
class resolver_cache;
class router;

namespace metrics {
class registry;
}

/// server class
class server : boost::noncopyable {
  public:
    explicit server();

    /// Return the lua engine
    lua_engine& get_lua_engine();

    /// Return the resolver cache
    resolver_cache& get_resolver_cache();

    /// Return the router
    router& get_router();

    /// Return the metrics registry
    metrics::registry& get_metrics_registry();

    /// Internal use
    server_impl* impl() { return m_impl; }

  private:
    server_impl* m_impl = nullptr;
};

}  // petrel

#endif  // SERVER_H
