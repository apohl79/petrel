/*
 * Copyright (c) 2016 Andreas Pohl
 * Licensed under MIT (see COPYING)
 *
 * Author: Andreas Pohl
 */

#ifndef FIBER_CACHE_H
#define FIBER_CACHE_H

#include <boost/asio.hpp>
#include <boost/core/noncopyable.hpp>
#include <boost/fiber/all.hpp>
#include <functional>
#include <memory>
#include <vector>

#include "branch.h"
#include "log.h"
#include "make_unique.h"

namespace petrel {

namespace ba = boost::asio;
namespace bf = boost::fibers;

/// A thread local cache for fiber objects that provides a simplistic run interface.
class fiber_cache : boost::noncopyable {
    set_log_tag_default_priority("fiber_cache");

  public:
    fiber_cache() {}

    /// Run a function in a fiber in the current thread context. If no free fiber is available a new one will be
    /// created.
    void run(std::function<void()>&& f);

    /// Register an io service object. As we are running one io service per worker we can use post() to execute cache
    /// updates in the context of each worker and maintain thread local caches that require no locks.
    ///
    /// @param iosvc The io service object to register
    void register_io_service(ba::io_service* iosvc);

    /// Unregister an io service object.
    ///
    /// @param iosvc The io service object to unregister
    void unregister_io_service(ba::io_service* iosvc);

  private:
    struct fiber_context : public std::enable_shared_from_this<fiber_context> {
        void start() {
            if (nullptr == fib) {
                auto self = shared_from_this();
                fib = std::make_unique<bf::fiber>([this, self] {
                    while (true) {
                        if (nullptr == func) {
                            // wait for new work
                            std::unique_lock<bf::mutex> lock(mtx);
                            cv.wait(lock);
                        }
                        if (likely(nullptr != func)) {
                            func();
                            func = nullptr;
                            if (likely(nullptr != fiber_cache::m_cache)) {
                                // put the fiber back into the local cache
                                fiber_cache::m_cache->push_back(self);
                            }
                        } else {
                            return;
                        }
                    }
                });
            }
        }

        void notify() { cv.notify_one(); }
        void join() { fib->join(); }

        std::unique_ptr<bf::fiber> fib;
        bf::mutex mtx;
        bf::condition_variable cv;
        std::function<void()> func = nullptr;
    };

    using cache_type = std::vector<std::shared_ptr<fiber_context>>;
    thread_local static std::unique_ptr<cache_type> m_cache;
    thread_local static std::uint_fast32_t m_fib_cnt;
    thread_local static bool m_stop;

    std::shared_ptr<fiber_context> get_fiber();
};

}  // petrel

#endif  // FIBER_CACHE_H
