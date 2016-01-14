/*
 * Copyright (c) 2016 Andreas Pohl
 * Licensed under MIT (see COPYING)
 *
 * Author: Andreas Pohl
 */

#ifndef METRICS_REGISTRY_H
#define METRICS_REGISTRY_H

#include <unordered_map>
#include <queue>
#include <memory>
#include <string>
#include <thread>
#include <atomic>

#include <boost/core/noncopyable.hpp>
#include <boost/asio.hpp>

#include "basic_metric.h"
#include "log.h"

namespace petrel {

class resolver_cache;

namespace metrics {

namespace ba = boost::asio;

std::ostream& operator<<(std::ostream& os, basic_metric& metric);

class registry : boost::noncopyable {
  public:
    set_log_tag_default_priority("metrics");

    registry(petrel::resolver_cache&);
    ~registry();

    /// Register a new metric
    template <class T>
    typename T::pointer register_metric(const std::string& name) {
        auto metric = get_metric<T>(name);
        if (nullptr == metric) {
            metric = std::make_shared<T>();
            m_metrics[name] = metric;
            m_new_metrics.push(metric);
        }
        return metric;
    }

    /// Get a metric from the registry
    template <class T>
    typename T::pointer get_metric(const std::string& name) const {
        auto it = m_metrics.find(name);
        if (m_metrics.end() != it) {
            auto metric = std::dynamic_pointer_cast<T>(it->second);
            return metric;
        }
        return nullptr;
    }

    /// Start the registry
    void start();

    /// Stop the registry
    void stop();

    /// Join the registry
    void join();

  private:
    int m_log_interval;

    bool m_graphite_enabled;
    int m_graphite_interval;
    std::string m_graphite_host;
    std::string m_graphite_port;
    std::atomic_bool m_stop{false};
    std::thread m_thread;

    std::unordered_map<std::string, basic_metric::pointer> m_metrics;
    std::queue<basic_metric::pointer> m_new_metrics;

    ba::io_service m_iosvc;
    petrel::resolver_cache& m_resolver;

    void run();
};

}  // metrics
}  // petrel

#endif  // METRICS_REGISTRY_H
