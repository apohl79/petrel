/*
 * Copyright (c) 2016 Andreas Pohl
 * Licensed under MIT (see COPYING)
 *
 * Author: Andreas Pohl
 */

#ifndef METRICS_TIMER_H
#define METRICS_TIMER_H

#include <algorithm>
#include <array>
#include <chrono>
#include <initializer_list>
#include <limits>
#include <memory>
#include <mutex>
#include <vector>

#include <petrel/core/log.h>
#include <petrel/metrics/basic_metric.h>

namespace petrel {
namespace metrics {

/// Timer
class timer : public basic_metric {
  public:
    using pointer = std::shared_ptr<timer>;

    /// A wrapper class to help with measuring a duration. Multiple timers can be assigned and updated simultanously.
    /// The class can be used for scope measurements too. The ctor will set the start time_point and the dtor will
    /// update the assigned timers.
    class duration {
        std::vector<timer::pointer> m_timers;
        std::chrono::high_resolution_clock::time_point m_start;
        bool m_finished = false;

      public:
        /// Create an object and assign a timer
        explicit duration(timer::pointer t) : m_start(std::chrono::high_resolution_clock::now()) {
            m_timers.push_back(t);
        }
        /// Create an object and assign a list of timers
        explicit duration(std::initializer_list<timer::pointer> l)
            : m_timers(l), m_start(std::chrono::high_resolution_clock::now()) {}
        ~duration() { update(); }
        /// Reset the start timer_point
        void reset() { m_start = std::chrono::high_resolution_clock::now(); }
        /// Measure the duration between now and the object creation or the last call to reset and update all timers.
        /// The start time_point will be set to now.
        void update() {
            if (!m_finished) {
                auto end = std::chrono::high_resolution_clock::now();
                for (auto t : m_timers) {
                    t->update(std::chrono::duration_cast<std::chrono::nanoseconds>(end - m_start).count());
                }
                m_start = end;
            }
        }
        /// Finish a duration measurement
        void finish() {
            update();
            m_finished = true;
        }
    };

    /// A histogram provides min, max, avg values as well as the distribution of the time values of a time frame.
    struct histogram {
        double min = 0;
        double max = 0;
        double avg = 0;
        double sum = 0;
        std::size_t count = 0;
        std::vector<std::pair<double, std::size_t>> dist;
        histogram(std::size_t num_of_bins, double bin_size) {
            double lower = 0;
            for (std::size_t i = 0; i < num_of_bins + 1; ++i) {
                dist.emplace_back(std::make_pair(lower, 0));
                lower += bin_size;
            }
        }
        void update_bin(std::size_t bin, std::size_t count) { dist[bin].second += count; }
    };

    /// Ctor.
    /// @param num_of_bins Number of bins for the underlying histogram calculation
    /// @param bin_size The size of each bin in nanoseconds
    timer(std::size_t num_of_bins = 10, double bin_size = 1000000) : m_num_of_bins(num_of_bins), m_bin_size(bin_size) {}

    void aggregate() {
        if (++m_aggregate_counter >= 10) {
            m_aggregate_counter = 0;
            auto& data = m_times[m_times_idx];
            {
                // switch to the other buffer
                std::lock_guard<std::mutex> lock(m_mtx);
                m_times_idx = (m_times_idx + 1) % 2;
            }
            histogram hist(m_num_of_bins, m_bin_size);
            if (!data.empty()) {
                std::sort(data.begin(), data.end());
                hist.min = data.front();
                hist.max = data.back();
                hist.count = data.size();
                for (auto t : data) {
                    hist.sum += t;
                }
                hist.avg = hist.sum / hist.count;

                // calc the distribution over m_num_of_bins bins with a size of m_bin_size nanoseconds
                std::size_t count = 0;
                std::size_t i = 0;
                auto upper = m_bin_size;
                for (auto d : data) {
                    if (d < upper + 1 || i == m_num_of_bins) {
                        // value fits into the bin
                        count++;
                    } else {
                        // create pair
                        hist.update_bin(i, count);
                        // next bin
                        while (d > upper && i < m_num_of_bins) {
                            i++;
                            upper += m_bin_size;
                        }
                        count = 1;
                    }
                }
                hist.update_bin(i, count);
                data.clear();
            }
            m_1min_values.push_back(std::move(hist));
            if (m_1min_values.size() > 6) {
                m_1min_values.erase(m_1min_values.begin());
            }
        }
    }

    histogram get_1min_histogram() {
        histogram aggregate(m_num_of_bins, m_bin_size);
        if (m_1min_values.size() > 0) {
            aggregate.min = std::numeric_limits<double>::max();
            for (auto& hist : m_1min_values) {
                aggregate.sum += hist.sum;
                aggregate.count += hist.count;
                for (std::size_t i = 0; i < m_num_of_bins + 1; ++i) {
                    aggregate.update_bin(i, hist.dist[i].second);
                }
                if (aggregate.min > hist.min) {
                    aggregate.min = hist.min;
                }
                if (aggregate.max < hist.max) {
                    aggregate.max = hist.max;
                }
            }
            if (aggregate.count > 0) {
                aggregate.avg = aggregate.sum / aggregate.count;
            }
        }
        return aggregate;
    }

    void log(std::ostream& os) {
        auto hist = get_1min_histogram();
        os << "1m  avg " << hist.avg / 1000000 << "ms, min " << hist.min / 1000000 << "ms, max " << hist.max / 1000000
           << "ms" << std::endl
           << log_tag("") << log_priority::info << "1m hist ";
        int count = 0;
        for (auto& p : hist.dist) {
            if (count > 0) {
                os << ", ";
            }
            double perc = 0;
            if (hist.count > 0) {
                perc = static_cast<double>(p.second) / hist.count * 100;
            }
            os << std::setw(5) << perc << "% >" << p.first / 1000000 << "ms";
            ++count;
            if (count % 5 == 0) {
                os << std::endl << log_tag("") << log_priority::info << "        ";
                count = 0;
            }
        }
    }

    void graphite(const std::string& name, std::time_t ts, std::ostream& os) {
        auto hist = get_1min_histogram();
        os << name << ".1m_avg_ms " << hist.avg / 1000000 << ' ' << ts << std::endl;
        os << name << ".1m_min_ms " << hist.min / 1000000 << ' ' << ts << std::endl;
        os << name << ".1m_max_ms " << hist.max / 1000000 << ' ' << ts << std::endl;
        os << name << ".1m_count " << hist.count << ' ' << ts << std::endl;
        for (auto& p : hist.dist) {
            double perc = 0;
            if (hist.count > 0) {
                perc = static_cast<double>(p.second) / hist.count * 100;
            }
            os << name << ".1m_dist_" << static_cast<int>(p.first / 1000000) << "ms_percent " << perc << ' ' << ts
               << std::endl;
            os << name << ".1m_dist_" << static_cast<int>(p.first / 1000000) << "ms_count " << p.second << ' ' << ts
               << std::endl;
        }
    }

    /// Add a time
    void update(double t) {
        std::lock_guard<std::mutex> lock(m_mtx);
        m_times[m_times_idx].push_back(t);
    }

  private:
    // use 2 buffers
    std::vector<double> m_times[2];
    std::uint8_t m_times_idx = 0;
    std::vector<histogram> m_1min_values;
    std::mutex m_mtx;
    std::size_t m_num_of_bins;
    double m_bin_size;
    std::uint8_t m_aggregate_counter = 0;
};

}  // metrics
}  // petrel

#endif  // METRICS_TIMER_H
