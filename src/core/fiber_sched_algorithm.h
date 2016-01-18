/*
 * Copyright (c) 2016 Andreas Pohl
 * Licensed under MIT (see COPYING)
 *
 * Author: Andreas Pohl
 *
 * Based of https://github.com/olk/boost-fiber/blob/master/examples/asio/fiber_sched_algorithm.hpp
 */

#ifndef FIBER_SCHED_ALGORITHM_H
#define FIBER_SCHED_ALGORITHM_H

#include <chrono>

#include <boost/asio.hpp>
#include <boost/assert.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/config.hpp>

#include <boost/fiber/context.hpp>
#include <boost/fiber/operations.hpp>
#include <boost/fiber/scheduler.hpp>

#include "boost/fiber/yield.hpp"

namespace petrel {

auto constexpr WAIT_INTERVAL_SHORT = std::chrono::nanoseconds(100);
auto constexpr WAIT_INTERVAL_LONG = std::chrono::milliseconds(1);
auto constexpr WAIT_INTERVAL_EXTRALONG = std::chrono::milliseconds(30);

class fiber_sched_algorithm : public boost::fibers::sched_algorithm {
  private:
    boost::asio::io_service& io_svc_;
    boost::asio::steady_timer suspend_timer_;
    boost::asio::steady_timer keepalive_timer_;
    std::chrono::steady_clock::duration keepalive_interval_;
    boost::fibers::scheduler::ready_queue_t ready_queue_{};

#ifndef __clang__
    thread_local static std::uint_fast64_t m_counter;
#else
    __thread static std::uint_fast64_t m_counter;
#endif

  public:
    fiber_sched_algorithm(boost::asio::io_service& io_svc) : io_svc_(io_svc), suspend_timer_(io_svc_), keepalive_timer_(io_svc_) {
        on_empty_io_service();
    }

    void awakened(boost::fibers::context* ctx) noexcept {
        BOOST_ASSERT(nullptr != ctx);
        BOOST_ASSERT(!ctx->ready_is_linked());
        ctx->ready_link(ready_queue_);
    }

    boost::fibers::context* pick_next() noexcept {
        boost::fibers::context* ctx(nullptr);
        if (!ready_queue_.empty()) {
            ctx = &ready_queue_.front();
            ready_queue_.pop_front();
            BOOST_ASSERT(nullptr != ctx);
            BOOST_ASSERT(!ctx->ready_is_linked());
        }
        return ctx;
    }

    bool has_ready_fibers() const noexcept { return !ready_queue_.empty(); }

    void suspend_until(std::chrono::steady_clock::time_point const& suspend_time) noexcept {
        suspend_timer_.expires_at(suspend_time);
        boost::system::error_code ignored_ec;
        suspend_timer_.async_wait(boost::fibers::asio::yield[ignored_ec]);
    }

    void notify() noexcept { suspend_timer_.expires_at(std::chrono::steady_clock::now()); }

    void on_empty_io_service() {
        io_svc_.post([]() { boost::this_fiber::yield(); });
        ++m_counter;
        auto expires = WAIT_INTERVAL_SHORT;
        if (m_counter > 55000) {
            expires = WAIT_INTERVAL_EXTRALONG;
        } else if (m_counter > 50000) {
            expires = WAIT_INTERVAL_LONG;
        }
        keepalive_timer_.expires_from_now(expires);
        keepalive_timer_.async_wait(std::bind(&fiber_sched_algorithm::on_empty_io_service, this));
    }

    /// Reset the timer counter to stay very responsive when we get requests
    static void reset_idle_counter();
};

}  // petrel

#endif  // FIBER_SCHED_ALGORITHM_H
