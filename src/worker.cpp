#include <boost/asio/high_resolution_timer.hpp>

#include "worker.h"
#include "server.h"
#include "options.h"
#include "log.h"
#include "boost/fiber/yield.hpp"

namespace petrel {

namespace bs = boost::system;
namespace bfa = bf::asio;

using ba::ip::tcp;

inline void timer_handler(ba::high_resolution_timer& timer, worker& w) {
    w.start_new_sessions();
    boost::this_fiber::yield();    
    timer.expires_from_now(bf::wait_interval());
    timer.async_wait(std::bind(timer_handler, std::ref(timer), std::ref(w)));
}

inline void run_service(ba::io_service& io_service, worker& w) {
    ba::high_resolution_timer timer(io_service, std::chrono::seconds(0));
    timer.async_wait(std::bind(timer_handler, std::ref(timer), std::ref(w)));
    // TODO: optimize, this takes too much CPU
    bf::wait_interval(std::chrono::nanoseconds(10));
    io_service.run();
}

worker::~worker() {
    join();
}

void worker::do_accept(tcp::acceptor& acceptor, server& srv) {
    for (;;) {
        bs::error_code ec;
        // get an io service to use for a new client, we pick them via round robin
        auto& worker = srv.get_worker();
        auto& iosvc = worker.io_service();
        auto new_session = std::make_shared<session>(srv, iosvc);
        acceptor.async_accept(new_session->socket(), bfa::yield[ec]);
        if (!ec) {
            worker.add_session(new_session);
        }
    }
}

void worker::add_endpoint(tcp::endpoint& ep) {
    try {
        auto acceptor = tcp::acceptor(m_iosvc);
        acceptor.open(ep.protocol());
        acceptor.set_option(tcp::acceptor::reuse_address(true));
        acceptor.bind(ep);
        int backlog = options::opts["server.backlog"].as<int>();
        if (backlog == 0) {
            backlog = ba::socket_base::max_connections;
        }
        acceptor.listen(backlog);
        m_acceptors.push_back(std::move(acceptor));
    } catch (bs::system_error& e) {
        log_err("failed to create acceptor: " << e.what());
    }
}

void worker::add_session(session::pointer new_session) {
    std::lock_guard<std::mutex> lock(m_new_session_mtx);
    m_new_sessions.push_back(new_session);
}

void worker::run(server& srv) {
    for (auto& acceptor : m_acceptors) {
        bf::fiber(worker::do_accept, std::ref(acceptor), std::ref(srv)).detach();
    }
    run_service(m_iosvc, *this);
}

void worker::join() {
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

void worker::start(server& srv) { m_thread = std::thread(&worker::run, this, std::ref(srv)); }

void worker::stop() { m_iosvc.stop(); }

}  // petrel
