/*
 * Copyright (c) 2016 Andreas Pohl
 * Licensed under MIT (see COPYING)
 *
 * Author: Andreas Pohl
 */

#ifndef METRICS_COUNTER_H
#define METRICS_COUNTER_H

#include <atomic>

#include <petrel/metrics/basic_metric.h>

namespace petrel {
namespace metrics {

/// Counter class
class counter : public basic_metric {
  public:
    using pointer = std::shared_ptr<counter>;

    /// Increment counter and return
    std::uint_fast64_t increment(std::uint_fast64_t i = 1) { return m_val.fetch_add(i, std::memory_order_relaxed) + i; }

    /// Decrement counter and return
    std::uint_fast64_t decrement(std::uint_fast64_t i = 1) { return m_val.fetch_sub(i, std::memory_order_relaxed) - i; }

    /// Return the current value and replace it by i
    std::uint_fast64_t exchange(std::uint_fast64_t i) { return m_val.exchange(i, std::memory_order_relaxed); }

    /// Return the current value and reset it to 0
    std::uint_fast64_t get_and_reset() { return m_val.exchange(0, std::memory_order_relaxed); }

    /// Return current total value
    std::uint_fast64_t get() const { return m_val; }

    void log(std::ostream& os) { os << m_val; }

    void graphite(const std::string& name, std::time_t ts, std::ostream& os) {
        os << name << ' ' << m_val << ' ' << ts << std::endl;
    }

  private:
    std::atomic_uint_fast64_t m_val{0};
};

}  // metrics
}  // petrel

#endif  // METRICS_COUNTER_H
