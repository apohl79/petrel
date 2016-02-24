/*
 * Copyright (c) 2016 Andreas Pohl
 * Licensed under MIT (see COPYING)
 *
 * Author: Andreas Pohl
 */

#include "resolver_cache.h"
#include "options.h"

namespace petrel {

resolver_cache::resolver_cache() {
    m_ttl = options::opts["server.dns-cache-ttl"].as<int>();
    log_info("using DNS cache TTL of " << m_ttl << " minutes");
}

template <>
resolver_cache::tcp_cache& resolver_cache::get_cache<resolver_cache::tcp>() {
    return m_tcp_cache;
}

template <>
resolver_cache::udp_cache& resolver_cache::get_cache<resolver_cache::udp>() {
    return m_udp_cache;
}

template <>
std::mutex& resolver_cache::get_cache_mtx<resolver_cache::tcp>() {
    return m_tcp_cache_mtx;
}

template <>
std::mutex& resolver_cache::get_cache_mtx<resolver_cache::udp>() {
    return m_udp_cache_mtx;
}

}  // petrel
