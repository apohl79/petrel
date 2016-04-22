/*
 * Copyright (c) 2016 Andreas Pohl
 * Licensed under MIT (see COPYING)
 *
 * Author: Andreas Pohl
 */

#include "fiber_sched_algorithm.h"

namespace petrel {

thread_local std::uint_fast64_t fiber_sched_algorithm::m_counter = 0;
thread_local std::uint_fast64_t fiber_sched_algorithm::m_requests = 0;
thread_local std::chrono::nanoseconds fiber_sched_algorithm::m_expires = WAIT_INTERVAL_SHORT;

void fiber_sched_algorithm::inc_requests() {
    m_requests++;
    m_counter = 0;
    m_expires = WAIT_INTERVAL_SHORT;
}

void fiber_sched_algorithm::dec_requests() {
    m_requests--;
    m_counter = 0;
    m_expires = WAIT_INTERVAL_SHORT;
}

}  // petrel
