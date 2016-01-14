/*
 * Copyright (c) 2016 Andreas Pohl
 * Licensed under MIT (see COPYING)
 *
 * Author: Andreas Pohl
 */

#ifndef FIBER_TIMER_H
#define FIBER_TIMER_H

#include <functional>
#include <boost/asio.hpp>

namespace petrel {

namespace ba = boost::asio;

/// A function type for the timer hook
using timer_hook_type = std::function<void()>;

/// Setup a timer to make fibers work with asio.
void setup_fiber_timer(ba::io_service& io_service);

/// Setup a timer to make fibers work with asio. Install a hook function that gets called by the timer handler.
void setup_fiber_timer(ba::io_service& io_service, timer_hook_type handler);

/// Reset the timer counter to stay very responsive when we get requests
void reset_fiber_idle_counter();

}  // petrel

#endif  // FIBER_TIMER_H
