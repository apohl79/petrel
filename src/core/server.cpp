/*
 * Copyright (c) 2016 Andreas Pohl
 * Licensed under MIT (see COPYING)
 *
 * Author: Andreas Pohl
 */

#include "server.h"
#include "server_impl.h"

namespace petrel {

server::server() : m_impl(new server_impl(this)) {}

lua_engine& server::get_lua_engine() { return m_impl->get_lua_engine(); }

resolver_cache& server::get_resolver_cache() { return m_impl->get_resolver_cache(); }

router& server::get_router() { return m_impl->get_router(); }

metrics::registry& server::get_metrics_registry() { return m_impl->get_metrics_registry(); }

}  // petrel
