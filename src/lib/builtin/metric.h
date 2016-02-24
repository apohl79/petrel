/*
 * Copyright (c) 2016 Andreas Pohl
 * Licensed under MIT (see COPYING)
 *
 * Author: Andreas Pohl
 */

#ifndef LIB_METRIC_H
#define LIB_METRIC_H

#include "library_builtin.h"
#include "metrics/basic_metric.h"
#include "metrics/timer.h"

namespace petrel {
namespace lib {

class metric : public library {
  public:
    enum class type { undef, counter, meter, timer };

    explicit metric(lib_context* ctx) : library(ctx), m_type(type::undef) {}

    /// Register a new counter metric.
    /// p1: string name
    int register_counter(lua_State* L);

    /// Register a new meter metric.
    /// p1: string name
    int register_meter(lua_State* L);

    /// Register a new timer metric.
    /// p1: string name
    int register_timer(lua_State* L);

    /// Load an existing counter metric.
    /// p1: string name
    int get_counter(lua_State* L);

    /// Load an existing meter metric.
    /// p1: string name
    int get_meter(lua_State* L);

    /// Load an existing timer metric.
    /// p1: string name
    int get_timer(lua_State* L);

    /// Update a timer metric.
    int update(lua_State* L);

    /// Start measuring a duration.
    int start_duration(lua_State* L);

    /// Measure a duration and reset the start time_point.
    int update_duration(lua_State* L);

    /// Measure a duration and remove the measurement object.
    int finish_duration(lua_State* L);

    /// Increment a counter or meter metric.
    int increment(lua_State* L);

    /// Decrement a counter.
    int decrement(lua_State* L);

    /// Get the total count of a metric or counter.
    int total(lua_State* L);

    /// Reset a counter.
    int reset(lua_State* L);

  private:
    type m_type;
    metrics::basic_metric::pointer m_metric;
    std::unique_ptr<metrics::timer::duration> m_duration;
};

}  // lib
}  // petrel

#endif  // LIB_METRIC_H
