/*
 * Copyright (c) 2016 Andreas Pohl
 * Licensed under MIT (see COPYING)
 *
 * Author: Andreas Pohl
 */

#include <unistd.h>
#include <boost/fiber/all.hpp>
#include <petrel/fiber/yield.hpp>

#include "server.h"
#include "server_impl.h"
#include "session.h"
#include "log.h"
#include "options.h"
#include "fiber_sched_algorithm.h"
#include "make_unique.h"

namespace petrel {

namespace bs = boost::system;
namespace bpo = boost::program_options;
namespace bf = boost::fibers;
namespace bfa = bf::asio;

server_impl::server_impl(server* srv) : m_server(srv), m_registry(m_resolver_cache) {
    m_num_workers = options::opts["server.workers"].as<int>();
    if (m_num_workers == 0) {
        m_num_workers = sysconf(_SC_NPROCESSORS_ONLN);
    }
    if (m_num_workers < 1) {
        m_num_workers = 1;
    }
    m_join_func = [] {};
    m_stop_func = [] {};
    m_metric_requests = m_registry.register_metric<metrics::meter>("requests");
    m_metric_errors = m_registry.register_metric<metrics::meter>("errors");
    m_metric_errors = m_registry.register_metric<metrics::meter>("not_implemented");
    m_metric_times = m_registry.register_metric<metrics::timer>("times");
}

void server_impl::init() {
    log_info("running " << m_num_workers << " workers");
    if (options::opts.count("server.http2")) {
        m_http2_server = std::make_unique<http2::server::http2>();
        if (m_num_workers > 1) {
            m_http2_server->num_threads(m_num_workers);
        }
    } else {
        for (int i = 0; i < m_num_workers; ++i) {
            m_workers.emplace_back(std::make_unique<worker>());
        }
    }

    std::vector<std::string> scripts;
    lua_engine::load_script_dir(options::opts["lua.root"].as<std::string>(), scripts);
    m_lua_engine.set_lua_scripts(scripts);
    m_lua_engine.bootstrap(*m_server);
    m_lua_engine.start(*m_server);

    if (m_num_routes == 0) {
        throw(std::runtime_error("No routes. You have to setup at least one route in your bootstrap function."));
    }

    m_registry.start();
}

void server_impl::start() {
    if (options::opts.count("server.http2")) {
        start_http2();
    } else {
        start_http();
    }
}

void server_impl::join() {
    m_join_func();
    m_lua_engine.join();
    m_registry.join();
}

void server_impl::stop() {
    log_notice("stopping server");
    m_stop_func();
    m_lua_engine.stop();
    m_registry.stop();
}

void server_impl::start_http2() {
    // install a handler that uses our own router
    m_http2_server->handle(
        "/", [this](const http2::server::request& req, const http2::server::response& res) {
            if (req.method() == "GET") {
                auto& route = m_router.find_route_http2(req.uri().raw_path);
                route(req, res, nullptr);
            } else if (req.method() == "POST") {
                log_debug("receiving content body");
                auto buf = std::make_shared<http2_content_buffer_type>();
                req.on_data([this, buf, &req, &res](const uint8_t* data, std::size_t len) {
                    if (len == 0) {
                        log_debug("received all content data");
                        // received all content, route the request now
                        auto& route = m_router.find_route_http2(req.uri().raw_path);
                        route(req, res, buf);
                    } else {
                        log_debug("received content chunk of " << len << " bytes");
                        // resize the buffer if needed
                        if (buf->capacity() - buf->size() < len) {
                            buf->reserve(buf->capacity() + len + 1024);
                        }
                        std::copy(data, data + len, std::back_inserter(*buf));
                    }
                });
            } else {
                m_metric_not_impl->increment();
                auto hm = http2::header_map();
                hm.emplace(std::make_pair(std::string("server"), http2::header_value{std::string("petrel"), false}));
                res.write_head(501, std::move(hm));
                res.end();
            }
        });
    // Check for SSL
    if (options::opts.count("server.http2.tls")) {
        run_http2_tls_server();
    } else {
        run_http2_server();
    }
}

void server_impl::start_http() {
    // Create acceptors
    using ba::ip::tcp;
    auto listen = options::opts["server.listen"].as<std::string>();
    auto port = options::opts["server.port"].as<std::string>();
    ba::io_service iosvc;
    tcp::resolver resolver(iosvc);
    tcp::resolver::query query(listen, port);
    bs::error_code ec;
    bool success = false;
    // Create an acceptor for any address and protocol type (IPv6/IPv4)
    for (auto it = resolver.resolve(query); it != tcp::resolver::iterator(); ++it) {
        tcp::endpoint ep = *it;
        bs::error_code ec_local;
        get_worker().add_endpoint(ep, ec_local);
        if (ec_local) {
            ec = ec_local;
        } else {
            success = true;
        }
    }
    if (!success) {
        throw std::runtime_error(ec.message());
    }
    // Run the io_service objects
    for (auto& w : m_workers) {
        w->start(*m_server);
    }
    log_notice("http server listening on " << listen << ":" << port);
    m_stop_func = [this] {
        // Stop the workers
        for (auto& w : m_workers) {
            w->stop();
        }
    };
    m_join_func = [this] {
        // Join the workers
        for (auto& w : m_workers) {
            w->join();
        }
    };
}

void server_impl::add_route_http2(const std::string& path, const std::string& func, metrics::meter::pointer metric_req,
                                  metrics::meter::pointer metric_err, metrics::timer::pointer metric_times) {
    m_router
        .add_route(
            path, [this, &path, func, metric_req, metric_times, metric_err](
                      const http2::server::request& req, const http2::server::response& res,
                      std::shared_ptr<http2_content_buffer_type> content) {
                log_debug("incomong request: method=" << req.method() << " path='" << req.uri().raw_path << "' query='"
                                                      << req.uri().raw_query << "' -> func=" << func);
                // reset the idle counter to stay responsive when we get traffic
                fiber_sched_algorithm::reset_idle_counter();
                // total requests
                m_metric_requests->increment();
                // path requests
                metric_req->increment();
                // timers (we create a shared_ptr of a duration and pass it into the fiber lambda, once the fiber
                // finishes,
                // the duration gets destroyed and updates the timers, so we measure the fiber lifetime)
                std::initializer_list<metrics::timer::pointer> l{m_metric_times, metric_times};
                auto times = std::make_shared<metrics::timer::duration>(l);
                try {
                    // create a fiber and run the request handler
                    bf::fiber([this, func, &req, content, &res, times, metric_err] {
                        try {
                            m_lua_engine.handle_http_request(func, req, res, *m_server, content);
                        } catch (std::runtime_error& e) {
                            m_metric_errors->increment();
                            metric_err->increment();
                            auto hm = http2::header_map();
                            hm.emplace(std::make_pair(std::string("server"),
                                                      http2::header_value{std::string("petrel"), false}));
                            res.write_head(500, std::move(hm));
                            res.end();
                        }
                    }).detach();
                } catch (std::runtime_error& e) {
                    m_metric_errors->increment();
                    metric_err->increment();
                    auto hm = http2::header_map();
                    hm.emplace(
                        std::make_pair(std::string("server"), http2::header_value{std::string("petrel"), false}));
                    res.write_head(500, std::move(hm));
                    res.end();
                }
            });
}

void server_impl::add_route_http(const std::string& path, const std::string& func, metrics::meter::pointer metric_req,
                                 metrics::meter::pointer metric_err, metrics::timer::pointer metric_times) {
    m_router.add_route(
        path, [this, &path, func, metric_req, metric_times, metric_err](session::request_type::pointer req) {
            log_debug("incomong request: method=" << req->method << " path='" << req->path << "' -> func = " << func);
            // reset the idle counter to stay responsive when we get traffic
            fiber_sched_algorithm::reset_idle_counter();
            // we support only GET/POST
            if (req->method == "GET" || req->method == "POST") {
                // total requests
                m_metric_requests->increment();
                // path requests
                metric_req->increment();
                // timers (we create a shared_ptr of a duration and pass it into the fiber lambda, once the fiber
                // finishes,
                // the duration gets destroyed and updates the timers, so we measure the fiber lifetime)
                std::initializer_list<metrics::timer::pointer> l{m_metric_times, metric_times};
                auto times = std::make_shared<metrics::timer::duration>(l);
                try {
                    // create a fiber and run the request handler
                    bf::fiber([this, func, req, times, metric_err]() mutable {
                        try {
                            m_lua_engine.handle_http_request(func, req);
                        } catch (std::runtime_error& e) {
                            m_metric_errors->increment();
                            metric_err->increment();
                            req->send_error_response(500);
                        }
                    }).detach();
                } catch (std::runtime_error& e) {
                    m_metric_errors->increment();
                    metric_err->increment();
                    req->send_error_response(500);
                }
            } else {
                m_metric_not_impl->increment();
                req->send_error_response(501);
            }
        });
}

void server_impl::add_route(const std::string& path, const std::string& func) {
    auto metric_req = m_registry.register_metric<metrics::meter>("requests_" + func);
    auto metric_err = m_registry.register_metric<metrics::meter>("errors_" + func);
    auto metric_times = m_registry.register_metric<metrics::timer>("times_" + func);
    if (options::opts.count("server.http2")) {
        add_route_http2(path, func, metric_req, metric_err, metric_times);
    } else {
        add_route_http(path, func, metric_req, metric_err, metric_times);
    }
    m_num_routes++;
    log_info("  new route: " << path << " -> " << func);
}

void server_impl::run_http2_server() {
    bs::error_code ec;
    if (m_http2_server->listen_and_serve(ec, options::opts["server.listen"].as<std::string>(),
                                         options::opts["server.port"].as<std::string>(), true)) {
        throw std::runtime_error(ec.message());
    }
    for (auto iosvc : m_http2_server->io_services()) {
        // register asio scheduler
        iosvc->post([iosvc] { bf::use_scheduling_algorithm<fiber_sched_algorithm>(*iosvc); });
    }
    log_notice("http2 server listening on " << options::opts["server.listen"].as<std::string>() << ":"
                                            << options::opts["server.port"].as<std::string>());
    m_stop_func = [this] { m_http2_server->stop(); };
    m_join_func = [this] { m_http2_server->join(); };
}

void server_impl::run_http2_tls_server() {
    log_notice("enabling https support");
    if (!options::opts.count("server.http2.key-file")) {
        throw std::invalid_argument("You have to specify a private key file (--server.http2.key-file) to use SSL/TLS.");
    }
    std::string key = options::opts["server.http2.key-file"].as<std::string>();
    if (!options::opts.count("server.http2.crt-file")) {
        throw std::invalid_argument("You have to specify a certificate file (--server.http2.crt-file) to use SSL/TLS.");
    }
    std::string crt = options::opts["server.http2.crt-file"].as<std::string>();
    log_notice("  using private key: " << key);
    log_notice("  using certificate: " << crt);

    m_tls = std::make_shared<ba::ssl::context>(ba::ssl::context::sslv23);
    m_tls->use_private_key_file(key, ba::ssl::context::pem);
    m_tls->use_certificate_chain_file(crt);

    bs::error_code ec;
    http2::server::configure_tls_context_easy(ec, *m_tls);
    if (bs::errc::success != ec.value()) {
        throw std::runtime_error(ec.message());
    }

    if (m_http2_server->listen_and_serve(ec, *m_tls, options::opts["server.listen"].as<std::string>(),
                                         options::opts["server.port"].as<std::string>(), true)) {
        throw std::runtime_error(ec.message());
    }
    for (auto iosvc : m_http2_server->io_services()) {
        // register asio scheduler
        iosvc->post([iosvc] { bf::use_scheduling_algorithm<fiber_sched_algorithm>(*iosvc); });
    }
    log_notice("http2 server listening on " << options::opts["server.listen"].as<std::string>() << ":"
                                            << options::opts["server.port"].as<std::string>());
    m_stop_func = [this] { m_http2_server->stop(); };
    m_join_func = [this] { m_http2_server->join(); };
}

}  // petrel
