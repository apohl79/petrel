/*
 * Copyright (c) 2016 Andreas Pohl
 * Licensed under MIT (see COPYING)
 *
 * Author: Andreas Pohl
 */

#include <boost/fiber/all.hpp>
#include <petrel/fiber/yield.hpp>
#include <random>
#include <unistd.h>

#include "fiber_sched_algorithm.h"
#include "log.h"
#include "lua_utils.h"
#include "make_unique.h"
#include "options.h"
#include "request.h"
#include "server.h"
#include "server_impl.h"
#include "session.h"

namespace petrel {

namespace bs = boost::system;
namespace bpo = boost::program_options;
namespace bf = boost::fibers;
namespace bfa = bf::asio;

/// Thread safe int rand
std::uint16_t int_rand(std::uint16_t min, std::uint16_t max) {
    static thread_local std::minstd_rand* gen = nullptr;
    if (unlikely(nullptr == gen)) {
        gen = new std::minstd_rand;
    }
    std::uniform_int_distribution<std::uint16_t> dist(min, max);
    return dist(*gen);
}

server_impl::server_impl(server* srv) : m_server(srv), m_registry(m_resolver_cache) {
    m_num_workers = options::get_int("server.workers", 1);
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
    if (!options::is_set("server.http1")) {
        m_http2_server = std::make_unique<http2::server::http2>();
        if (m_num_workers > 1) {
            m_http2_server->num_threads(m_num_workers);
        }
    } else {
        for (int i = 0; i < m_num_workers; ++i) {
            m_workers.emplace_back(std::make_unique<worker>());
        }
    }

    m_lua_engine.bootstrap(*m_server);

    if (m_num_routes == 0) {
        throw(std::runtime_error("No routes. You have to setup at least one route in your bootstrap function."));
    }

    m_registry.start();
}

void server_impl::start() {
    if (!options::is_set("server.http1")) {
        start_http2();
    } else {
        start_http();
    }
}

void server_impl::join() {
    m_join_func();
    m_registry.join();
}

void server_impl::stop() {
    log_notice("stopping server");
    m_stop_func();
    m_registry.stop();
}

void server_impl::start_http2() {
    // install a handler that uses our own router
    m_http2_server->handle("/", [this](const http2::server::request& req, const http2::server::response& res) {
        if (req.method() == "GET") {
            auto& route = m_router.find_route(req.uri().raw_path);
            route(std::make_shared<request>(req, res, *m_server, nullptr));
        } else if (req.method() == "POST") {
            log_debug("receiving content body");
            auto buf = std::make_shared<request::http2_content_buffer_type>();
            req.on_data([this, buf, &req, &res](const uint8_t* data, std::size_t len) {
                if (len == 0) {
                    log_debug("received all content data");
                    // received all content, route the request now
                    auto& route = m_router.find_route(req.uri().raw_path);
                    route(std::make_shared<request>(req, res, *m_server, buf));
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
            request r(req, res, *m_server, nullptr);
            r.send_error_response(501);
        }
    });
    // Check for SSL
    if (options::is_set("server.tls")) {
        run_http2_tls_server();
    } else {
        run_http2_server();
    }
}

void server_impl::start_http() {
    // Create acceptors
    using ba::ip::tcp;
    auto listen = options::get_string("server.listen");
    auto port = options::get_string("server.port");
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
        // create a thread local lua state cache
        m_lua_engine.state_manager().register_io_service(&w->io_service());
        // create a thread local file cache
        m_file_cache.register_io_service(&w->io_service());
    }
    log_notice("http server listening on " << listen << ":" << port);
    m_stop_func = [this] {
        // Stop the workers
        for (auto& w : m_workers) {
            m_lua_engine.state_manager().unregister_io_service(&w->io_service());
            m_file_cache.unregister_io_service(&w->io_service());
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

void server_impl::add_route(const std::string& path, const std::string& func) {
    auto metric_req = m_registry.register_metric<metrics::meter>("requests_" + func);
    auto metric_err = m_registry.register_metric<metrics::meter>("errors_" + func);
    auto metric_times = m_registry.register_metric<metrics::timer>("times_" + func);
    m_router.add_route(path, [this, func, metric_req, metric_times, metric_err](request::pointer req) {
        log_debug("incomong request: method=" << req->method_string() << " path='" << req->path()
                                              << "' -> func=" << func);
        // reset the idle counter to stay responsive when we get traffic
        fiber_sched_algorithm::reset_idle_counter();
        // we support only GET/POST
        if (req->method() == request::http_method::GET || req->method() == request::http_method::POST) {
            // total requests
            m_metric_requests->increment();
            // path requests
            metric_req->increment();
            // timers (we create a shared_ptr of a duration and pass it into the fiber lambda, once the fiber
            // finishes, the duration gets destroyed and updates the timers, so we measure the fiber lifetime)
            // (measure time with 10% sampling to keep the impact low)
            std::shared_ptr<metrics::timer::duration> times;
            auto rnd = int_rand(0, 100);
            if (unlikely(rnd < 10)) {
                std::initializer_list<metrics::timer::pointer> l{m_metric_times, metric_times};
                times = std::make_shared<metrics::timer::duration>(l);
            }
            try {
                // create a fiber and run the request handler
                bf::fiber([this, func, req, times, metric_err] {
                    try {
                        m_lua_engine.handle_request(func, req);
                    } catch (std::runtime_error& e) {
                        log_debug("handle_request failed: " << e.what());
                        m_metric_errors->increment();
                        metric_err->increment();
                        req->send_error_response(500);
                    }
                }).detach();
            } catch (std::runtime_error& e) {
                log_debug("fiber failed: " << e.what());
                m_metric_errors->increment();
                metric_err->increment();
                req->send_error_response(500);
            }
        } else {
            m_metric_not_impl->increment();
            req->send_error_response(501);
        }
    });
    m_num_routes++;
    log_info("  new route: " << path << " -> " << func);
}

void server_impl::add_directory_route(const std::string& path, const std::string& dir) {
    if (m_file_cache.add_directory(dir)) {
        auto metric_req = m_registry.register_metric<metrics::meter>("requests_static_files");
        auto metric_err = m_registry.register_metric<metrics::meter>("errors_static_files");
        auto metric_times = m_registry.register_metric<metrics::timer>("times_static_files");
        m_router.add_route(path, [this, path, dir, metric_req, metric_times, metric_err](request::pointer req) {
            log_debug("incomong request: method=" << req->method_string() << " path='" << req->path()
                                                  << "' -> static_dir=" << dir);
            fiber_sched_algorithm::reset_idle_counter();
            if (req->method() == request::http_method::GET) {
                // measure time with 10% sampling to keep the impact low
                std::unique_ptr<metrics::timer::duration> times;
                auto rnd = int_rand(0, 100);
                if (unlikely(rnd < 10)) {
                    std::initializer_list<metrics::timer::pointer> l{m_metric_times, metric_times};
                    times = std::make_unique<metrics::timer::duration>(l);
                }
                auto file = find_static_file(dir, path, req->path());
                if (nullptr != file) {
                    req->send_response(200, boost::string_ref(file->data().data(), file->size()));
                    m_metric_requests->increment();
                    metric_req->increment();
                    return;
                }
            }
            m_metric_errors->increment();
            metric_err->increment();
            req->send_error_response(404);
        });
        m_num_routes++;
        log_info("  new route: " << path << " -> static_dir:" << dir);
    }
}

std::shared_ptr<file_cache::file> server_impl::find_static_file(const std::string& dir, const std::string& path,
                                                                const std::string& req_path) {
    if (req_path[req_path.size() - 1] == '/') {
        return nullptr;
    }
    auto file = dir;
    file += req_path.substr(path.size());
    auto ret = m_file_cache.get_file(file);
    if (nullptr != ret) {
        return ret;
    }
    log_debug("file " << file << " not found");
    return nullptr;
}

void server_impl::run_http2_server() {
    bs::error_code ec;
    if (m_http2_server->listen_and_serve(ec, options::get_string("server.listen"), options::get_string("server.port"),
                                         true)) {
        throw std::runtime_error(ec.message());
    }
    for (auto iosvc : m_http2_server->io_services()) {
        // register asio scheduler
        iosvc->post([iosvc] { bf::use_scheduling_algorithm<fiber_sched_algorithm>(*iosvc); });
        // create a thread local lua state cache
        m_lua_engine.state_manager().register_io_service(iosvc.get());
        // create a thread local file cache
        m_file_cache.register_io_service(iosvc.get());
    }
    log_notice("http2 server listening on " << options::get_string("server.listen") << ":"
                                            << options::get_string("server.port"));
    m_stop_func = [this] {
        for (auto iosvc : m_http2_server->io_services()) {
            m_lua_engine.state_manager().unregister_io_service(iosvc.get());
            m_file_cache.unregister_io_service(iosvc.get());
        }
        m_http2_server->stop();
    };
    m_join_func = [this] { m_http2_server->join(); };
}

void server_impl::run_http2_tls_server() {
    log_notice("enabling https support");
    if (!options::is_set("server.key-file")) {
        throw std::invalid_argument("You have to specify a private key file (--server.key-file) to use SSL/TLS.");
    }
    std::string key = options::get_string("server.key-file");
    if (!options::is_set("server.crt-file")) {
        throw std::invalid_argument("You have to specify a certificate file (--server.crt-file) to use SSL/TLS.");
    }
    std::string crt = options::get_string("server.crt-file");
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

    if (m_http2_server->listen_and_serve(ec, *m_tls, options::get_string("server.listen"),
                                         options::get_string("server.port"), true)) {
        throw std::runtime_error(ec.message());
    }
    for (auto iosvc : m_http2_server->io_services()) {
        // register asio scheduler
        iosvc->post([iosvc] { bf::use_scheduling_algorithm<fiber_sched_algorithm>(*iosvc); });
        // create a thread local lua state cache
        m_lua_engine.state_manager().register_io_service(iosvc.get());
        // create a thread local file cache
        m_file_cache.register_io_service(iosvc.get());
    }
    log_notice("http2 server listening on " << options::get_string("server.listen") << ":"
                                            << options::get_string("server.port"));
    m_stop_func = [this] {
        for (auto iosvc : m_http2_server->io_services()) {
            m_lua_engine.state_manager().unregister_io_service(iosvc.get());
            m_file_cache.unregister_io_service(iosvc.get());
        }
        m_http2_server->stop();
    };
    m_join_func = [this] { m_http2_server->join(); };
}

}  // petrel
