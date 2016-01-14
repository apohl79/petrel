/*
 * Copyright (c) 2016 Andreas Pohl
 * Licensed under MIT (see COPYING)
 *
 * Author: Andreas Pohl
 */

#include "registry.h"
#include "options.h"
#include "resolver_cache.h"
#include "boost/fiber/yield.hpp"

#include <ostream>
#include <ctime>

#include <boost/fiber/all.hpp>
#include <boost/asio/high_resolution_timer.hpp>

namespace petrel {
namespace metrics {

namespace bf = boost::fibers;
namespace bfa = bf::asio;
namespace bai = ba::ip;
namespace bs = boost::system;

inline void timer_handler(ba::high_resolution_timer& timer) {
    boost::this_fiber::yield();
    timer.expires_from_now(bf::wait_interval());
    timer.async_wait(std::bind(timer_handler, std::ref(timer)));
}

inline void run_service(ba::io_service& io_service) {
    ba::high_resolution_timer timer(io_service, std::chrono::seconds(0));
    timer.async_wait(std::bind(timer_handler, std::ref(timer)));
    io_service.run();
}

registry::registry(petrel::resolver_cache& resolver)
    : m_log_interval(options::opts["metrics.log"].as<int>()),
      m_graphite_enabled(options::opts.count("metrics.graphite")),
      m_graphite_interval(options::opts["metrics.graphite.interval"].as<int>()),
      m_resolver(resolver) {
    if (m_log_interval > 0) {
        log_info("logging metrics every " << m_log_interval << " seconds");
    }
    if (m_graphite_enabled && m_graphite_interval > 0) {
        if (options::opts.count("metrics.graphite.host") && options::opts.count("metrics.graphite.port")) {
            m_graphite_host = options::opts["metrics.graphite.host"].as<std::string>();
            m_graphite_port = options::opts["metrics.graphite.port"].as<std::string>();
            log_info("sending metrics every " << m_graphite_interval << " seconds to graphite://" << m_graphite_host
                                              << ":" << m_graphite_port);
        } else {
            log_warn("graphite enabled but no host/port defined. disabling graphite.");
            m_graphite_enabled = false;
        }
    }
}

registry::~registry() {
    join();
}

void registry::start() { m_thread = std::thread(&registry::run, this); }

void registry::join() {
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

void registry::stop() {
    m_stop = true;
    m_iosvc.stop();
}

void registry::run() {
    // If metrics logging is enabled, we log all metrics every N seconds
    if (m_log_interval > 0) {
        bf::fiber([this] {
            std::string prefix = "m:";
            while(!m_stop) {
                boost::this_fiber::sleep_for(std::chrono::seconds(m_log_interval));
                log_info("----- metrics -----");
                for (auto metric : m_metrics) {
                    if (metric.second->visible_in_log()) {
                        set_log_tag(prefix + metric.first);
                        log_info(*metric.second);
                    }
                }
                log_info("-------------------");
            }
        }).detach();
    }
    // Graphite logging
    if (m_graphite_enabled) {
        bf::fiber([this] {
            std::string prefix =
                options::opts["metrics.graphite.prefix"].as<std::string>() + "." + bai::host_name() + ".";
            while (!m_stop) {
                boost::this_fiber::sleep_for(std::chrono::seconds(m_graphite_interval));
                try {
                    // connecting carbon
                    bai::tcp::socket sock(m_iosvc);
                    auto ep_iter = m_resolver.async_resolve<resolver_cache::tcp>(
                        m_iosvc, m_graphite_host, m_graphite_port, bfa::yield);
                    ba::async_connect(sock, ep_iter, bfa::yield);
                    // send each metric
                    ba::streambuf buf;
                    std::ostream os(&buf);
                    auto ts = std::time(nullptr);
                    for (auto metric : m_metrics) {
                        std::string name = prefix + metric.first;
                        metric.second->graphite(name, ts, os);
                    }
                    ba::async_write(sock, buf, bfa::yield);
                } catch (bs::system_error& e) {
                    log_err("sending metrics to graphite failed: " << e.what());
                }
            }
        }).detach();
    }
    // Check for new metrics to run their aggregate functions
    bf::fiber([this] {
        while (!m_stop) {
            boost::this_fiber::sleep_for(std::chrono::seconds(1));
            while (!m_new_metrics.empty()) {
                auto metric = m_new_metrics.front();
                m_new_metrics.pop();
                bf::fiber([metric] { metric->aggregate(); }).detach();
            }
        }
    }).detach();
    // Run the io service
    run_service(m_iosvc);
}

std::ostream& operator<<(std::ostream& os, basic_metric& metric) {
    metric.log(os);
    return os;
}

}  // metrics
}  // petrel
