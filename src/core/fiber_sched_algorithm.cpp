/*
 * Copyright (c) 2016 Andreas Pohl
 * Licensed under MIT (see COPYING)
 *
 * Author: Andreas Pohl
 */

#include "fiber_sched_algorithm.h"

namespace petrel {

#ifndef __clang__
thread_local std::uint_fast64_t fiber_sched_algorithm::m_counter = 0;
#else
__thread std::uint_fast64_t fiber_sched_algorithm::m_counter = 0;
#endif

void fiber_sched_algorithm::reset_idle_counter() { m_counter = 0; }

}  // petrel
