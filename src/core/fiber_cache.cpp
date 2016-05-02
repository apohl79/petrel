/*
 * Copyright (c) 2016 Andreas Pohl
 * Licensed under MIT (see COPYING)
 *
 * Author: Andreas Pohl
 */

#include "fiber_cache.h"
#include "asio_post.h"

namespace petrel {

thread_local std::unique_ptr<fiber_cache::cache_type> fiber_cache::m_cache;
thread_local std::uint_fast32_t fiber_cache::m_fib_cnt = 0;
thread_local bool fiber_cache::m_stop = false;

void fiber_cache::run(std::function<void()>&& f) {
    auto fctx = get_fiber();
    if (likely(nullptr != fctx)) {
        fctx->func = f;
        fctx->notify();
    } else {
        // no cache available (shutdown in progress), now create a fiber directly
        bf::fiber(f).detach();
    }
}

void fiber_cache::register_io_service(ba::io_service* iosvc) {
    iosvc->post([] { m_cache = std::make_unique<cache_type>(); });
}

void fiber_cache::unregister_io_service(ba::io_service* iosvc) {
    // wait for all active fiber contexts to return to the cache
    io_service_post_wait(iosvc, [] { m_stop = true; });
    bool no_active_fctx = false;
    do {
        io_service_post_wait(iosvc, [&no_active_fctx] { no_active_fctx = m_fib_cnt == m_cache->size(); });
        if (!no_active_fctx) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    } while (!no_active_fctx);
    // cleanup
    io_service_post_wait(iosvc, [] {
        for (auto fctx : *m_cache) {
            fctx->notify();
            fctx->join();
        }
        m_cache.reset();
    });
}

std::shared_ptr<fiber_cache::fiber_context> fiber_cache::get_fiber() {
    if (unlikely(m_stop || nullptr == m_cache)) {
        return nullptr;
    }
    if (likely(!m_cache->empty())) {
        auto fctx = m_cache->back();
        m_cache->pop_back();
        return fctx;
    }
    auto fctx = std::make_shared<fiber_cache::fiber_context>();
    fctx->start();
    m_fib_cnt++;
    return fctx;
}

}  // petrel
