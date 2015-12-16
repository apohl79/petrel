#include "fiber_timer.h"

#include <memory>
#include <boost/fiber/all.hpp>
#include <boost/asio/high_resolution_timer.hpp>

namespace petrel {

namespace bf = boost::fibers;

auto constexpr WAIT_INTERVAL_SHORT = std::chrono::nanoseconds(100);
auto constexpr WAIT_INTERVAL_LONG = std::chrono::milliseconds(1);
auto constexpr WAIT_INTERVAL_EXTRALONG = std::chrono::milliseconds(30);

#ifndef __clang__
thread_local std::uint_fast64_t counter = 0;
#else
__thread std::uint_fast64_t counter = 0;
#endif

inline void timer_handler(std::shared_ptr<ba::high_resolution_timer> timer, timer_hook_type hook) {
    hook();
    boost::this_fiber::yield();
    ++counter;
    auto expires = WAIT_INTERVAL_SHORT;
    if (counter > 55000) {
        expires = WAIT_INTERVAL_EXTRALONG;
    } else if (counter > 50000) {
        expires = WAIT_INTERVAL_LONG;
    }
    timer->expires_from_now(expires);
    timer->async_wait(std::bind(timer_handler, timer, std::move(hook)));
}

void setup_fiber_timer(ba::io_service& io_service) {
    setup_fiber_timer(io_service, [] {});
}

void setup_fiber_timer(ba::io_service& io_service, timer_hook_type hook) {
    auto timer = std::make_shared<ba::high_resolution_timer>(io_service, std::chrono::seconds(0));
    timer->async_wait(std::bind(timer_handler, timer, std::move(hook)));
}

void reset_fiber_idle_counter() { counter = 0; }

}  // petrel
