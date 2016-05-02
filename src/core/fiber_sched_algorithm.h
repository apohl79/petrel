/*
 * Copyright (c) 2016 Andreas Pohl
 * Licensed under MIT (see COPYING)
 *
 * Author: Andreas Pohl
 *
 * Based of https://github.com/olk/boost-fiber/blob/master/examples/asio/round_robin.hpp
 */

#ifndef FIBER_SCHED_ALGORITHM_H
#define FIBER_SCHED_ALGORITHM_H

#include <chrono>

#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/assert.hpp>
#include <boost/config.hpp>

#include <boost/fiber/context.hpp>
#include <boost/fiber/operations.hpp>
#include <boost/fiber/scheduler.hpp>

#include <petrel/fiber/yield.hpp>

#include "branch.h"
#include "log.h"

namespace petrel {

auto constexpr WAIT_INTERVAL_SHORT = std::chrono::nanoseconds(100);
auto constexpr WAIT_INTERVAL_LONG = std::chrono::milliseconds(1);
auto constexpr WAIT_INTERVAL_EXTRALONG = std::chrono::milliseconds(30);

class fiber_sched_algorithm : public boost::fibers::sched_algorithm {
  private:
    boost::asio::io_service& m_iosvc;
    boost::asio::steady_timer m_suspend_timer;
    boost::asio::steady_timer m_keepalive_timer;
    boost::fibers::scheduler::ready_queue_t m_ready_queue{};

    thread_local static std::uint_fast64_t m_counter;
    thread_local static double m_rate_l;
    thread_local static double m_rate_xl;
    thread_local static std::chrono::nanoseconds m_expires;

  public:
    explicit fiber_sched_algorithm(boost::asio::io_service& iosvc)
        : m_iosvc(iosvc), m_suspend_timer(iosvc), m_keepalive_timer(iosvc) {
        on_empty_io_service();
    }

    void awakened(boost::fibers::context* ctx) noexcept {
        BOOST_ASSERT(nullptr != ctx);
        BOOST_ASSERT(!ctx->ready_is_linked());
        ctx->ready_link(m_ready_queue);
    }

    boost::fibers::context* pick_next() noexcept {
        boost::fibers::context* ctx(nullptr);
        if (!m_ready_queue.empty()) {
            ctx = &m_ready_queue.front();
            m_ready_queue.pop_front();
            BOOST_ASSERT(nullptr != ctx);
            BOOST_ASSERT(!ctx->ready_is_linked());
        }
        return ctx;
    }

    bool has_ready_fibers() const noexcept { return !m_ready_queue.empty(); }

    void suspend_until(std::chrono::steady_clock::time_point const& suspend_time) noexcept {
        m_suspend_timer.expires_at(suspend_time);
        boost::system::error_code ignored_ec;
        m_suspend_timer.async_wait(boost::fibers::asio::yield[ignored_ec]);
    }

    void notify() noexcept { m_suspend_timer.expires_at(std::chrono::steady_clock::now()); }

    void on_empty_io_service() {
        m_iosvc.post([]() { boost::this_fiber::yield(); });

        m_rate_l = m_rate_l * 0.99999 - 0.0001 + m_counter;
        m_rate_xl = m_rate_xl * 0.99999 - 0.00009 + m_counter;
        m_counter = 0;

        if (m_rate_l < 0.0) {
            m_rate_l = 0.0;
        }
        if (m_rate_xl < 0.0) {
            m_rate_xl = 0.0;
        }

        if (unlikely(m_rate_xl == 0.0)) {
            if (unlikely(m_expires != WAIT_INTERVAL_EXTRALONG)) {
                m_expires = WAIT_INTERVAL_EXTRALONG;
            }
        } else if (unlikely(m_rate_l == 0.0)) {
            if (unlikely(m_expires != WAIT_INTERVAL_LONG)) {
                m_expires = WAIT_INTERVAL_LONG;
            }
        } else if (unlikely(m_expires != WAIT_INTERVAL_SHORT)) {
            m_expires = WAIT_INTERVAL_SHORT;
        }

        m_keepalive_timer.expires_from_now(m_expires);
        m_keepalive_timer.async_wait(std::bind(&fiber_sched_algorithm::on_empty_io_service, this));
    }

    static void update();
};

}  // petrel

#endif  // FIBER_SCHED_ALGORITHM_H
