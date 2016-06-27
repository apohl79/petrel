/*
 * Copyright (c) 2016 Andreas Pohl
 * Licensed under MIT (see COPYING)
 *
 * Author: Andreas Pohl
 */

#include <petrel/fiber/yield.hpp>

#include "fiber_sched_algorithm.h"
#include "options.h"
#include "server.h"
#include "server_impl.h"
#include "worker.h"

namespace petrel {

namespace bfa = bf::asio;

using ba::ip::tcp;

worker::~worker() {
    stop();
    join();
}

void worker::do_accept(tcp::acceptor& acceptor, server& srv) {
    while (acceptor.is_open()) {
        bs::error_code ec;
        // get an io service to use for a new client, we pick them via round robin
        auto& worker = srv.impl()->get_worker();
        auto& iosvc = worker.io_service();
        auto new_session = std::make_shared<session>(srv, iosvc);
        acceptor.async_accept(new_session->socket(), bfa::yield[ec]);
        if (!ec) {
            worker.add_session(new_session);
            worker.m_new_session_cv.notify_one();
        }
    }
}

void worker::add_endpoint(tcp::endpoint& ep, bs::error_code& ec) {
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
        ec = e.code();
    }
}

void worker::add_session(session::pointer new_session) {
    std::unique_lock<bf::mutex> lock(m_new_session_mtx);
    m_new_sessions.push_back(new_session);
}

void worker::run(server& srv) {
    for (auto& acceptor : m_acceptors) {
        bf::fiber(worker::do_accept, std::ref(acceptor), std::ref(srv)).detach();
    }
    bf::fiber([this] {
        while (!m_stop) {
            std::unique_lock<bf::mutex> lock(m_new_session_mtx);
            m_new_session_cv.wait(lock);
            for (auto session : m_new_sessions) {
                bf::fiber(&session::start, session).detach();
            }
            m_new_sessions.clear();
        }
    }).detach();
    // Run the io service
    bf::use_scheduling_algorithm<fiber_sched_algorithm>(m_iosvc);
    m_iosvc.run();
}

void worker::join() {
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

void worker::start(server& srv) { m_thread = std::thread(&worker::run, this, std::ref(srv)); }

void worker::stop() {
    if (!m_stop) {
        m_stop = true;
        m_new_session_cv.notify_one();
        for (auto& acceptor : m_acceptors) {
            acceptor.close();
        }
        // let the acceptors finish
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        m_iosvc.stop();
    }
}

}  // petrel
