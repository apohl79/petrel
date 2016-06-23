/*
 * Copyright (c) 2016 Andreas Pohl
 * Licensed under MIT (see COPYING)
 *
 * Author: Andreas Pohl
 */

#ifndef METRICS_METER_H
#define METRICS_METER_H

#include <cmath>
#include <memory>

#include <petrel/metrics/counter.h>

namespace petrel {
namespace metrics {

/// Meter class which supports rates over 1min, 5min, 15min and 1hr intervals by implementing EWMA.
/// See: https://en.wikipedia.org/wiki/Moving_average#Exponential_moving_average
class meter : public basic_metric {
  public:
    using pointer = std::shared_ptr<meter>;

    meter()
        : ALPHA_1min(alpha(60)), ALPHA_5min(alpha(5 * 60)), ALPHA_15min(alpha(15 * 60)), ALPHA_1hr(alpha(60 * 60)) {}

    void aggregate() {
        auto c = m_counter_1s.get_and_reset();
        m_counter_total += c;
        m_rate_1min = m_rate_1min * (1 - ALPHA_1min) + c * ALPHA_1min;
        m_rate_5min = m_rate_5min * (1 - ALPHA_5min) + c * ALPHA_5min;
        m_rate_15min = m_rate_15min * (1 - ALPHA_15min) + c * ALPHA_15min;
        m_rate_1hr = m_rate_1hr * (1 - ALPHA_1hr) + c * ALPHA_1hr;
    }

    void log(std::ostream& os) { os << "1m rate " << m_rate_1min; }

    void graphite(const std::string& name, std::time_t ts, std::ostream& os) {
        os << name << ".1m_rate" << ' ' << m_rate_1min << ' ' << ts << std::endl;
        os << name << ".5m_rate" << ' ' << m_rate_5min << ' ' << ts << std::endl;
        os << name << ".15m_rate" << ' ' << m_rate_15min << ' ' << ts << std::endl;
        os << name << ".1hr_rate" << ' ' << m_rate_1hr << ' ' << ts << std::endl;
    }

    /// Increment counter and return
    std::uint_fast64_t increment(std::uint_fast64_t i = 1) { return m_counter_1s.increment(i); }

    /// Return total counter
    inline std::uint_fast64_t total() const { return m_counter_total; }

    /// Returm 1 minute rate
    inline double rate_1min() const { return m_rate_1min; }

    /// Returm 5 minute rate
    inline double rate_5min() const { return m_rate_5min; }

    /// Returm 15 minute rate
    inline double rate_15min() const { return m_rate_15min; }

    /// Returm 1 hour rate
    inline double rate_1hr() const { return m_rate_1hr; }

  private:
    counter m_counter_1s;
    std::uint_fast64_t m_counter_total = 0;
    double m_rate_1min = 0.0;
    double m_rate_5min = 0.0;
    double m_rate_15min = 0.0;
    double m_rate_1hr = 0.0;

    const double ALPHA_1min;
    const double ALPHA_5min;
    const double ALPHA_15min;
    const double ALPHA_1hr;

    /// Calculate the alpha for the given time interval in seconds
    inline double alpha(int secs) { return 1 - std::exp(std::log(0.005) / secs); }
};

}  // metrics
}  // petrel

#endif  // METRICS_METER_H
