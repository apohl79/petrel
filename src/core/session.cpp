/*
 * Copyright (c) 2016 Andreas Pohl
 * Licensed under MIT (see COPYING)
 *
 * Author: Andreas Pohl
 */

#include <boost/utility/string_ref.hpp>

#include "session.h"
#include "server.h"
#include "router.h"
#include "metrics/registry.h"
#include "metrics/meter.h"
#include "metrics/timer.h"
#include "boost/fiber/yield.hpp"
#include "boost/http/algorithm.hpp"

namespace petrel {

namespace bs = boost::system;
namespace bf = boost::fibers;
namespace bfa = bf::asio;

session::session(server& srv, ba::io_service& iosvc) : m_srv(srv), m_iosvc(iosvc), m_socket(m_iosvc) {}

session::~session() {}

server& session::get_server() { return m_srv; }

ba::io_service& session::get_io_service() { return m_iosvc; }

void session::start() {
    auto self = shared_from_this();  // make sure the object stays alive until the fiber exits
    while (m_socket.is_open()) {
        try {
            // add the req to the queue to wait for sending back a response
            auto req = std::make_shared<request>(self);
            req->remote_endpoint = m_socket.next_layer().remote_endpoint();
            // read the request
            m_socket.async_read_request(req->method, req->path, req->message, bfa::yield);
            if (http::request_continue_required(req->message)) {
                m_socket.async_write_response_continue(bfa::yield);
            }
            while (m_socket.read_state() != http::read_state::empty) {
                switch (m_socket.read_state()) {
                    case http::read_state::message_ready:
                        m_socket.async_read_some(req->message, bfa::yield);
                        break;
                    case http::read_state::body_ready:
                        m_socket.async_read_trailers(req->message, bfa::yield);
                        break;
                    default:;
                }
            }
            // find a handler and execute it
            auto& route = m_srv.get_router().find_route_http(req->path);
            route(req);
            // wait for the response to become ready
            // TODO: implement parallel pipeline request processing by queing up responses, as we need to preserve the
            // order. boost.http does not support this, see socket-inl.hpp:167. Once we call async_read_request again,
            // async_write_response fails.
            req->wait();
            send_response(req->response);
        } catch (bs::system_error& e) {
            // TODO: move into metric
            if (e.code() != ba::error::eof && e.code() != ba::error::operation_aborted &&
                e.code() != ba::error::connection_reset) {
                log_err(e.what());
            } else {
                break;
            }
        }
    }
}

void session::send_response(response_type& res) {
    try {
        auto sc = http::status_code(res.status);
        m_socket.async_write_response(res.status, http::to_string<boost::string_ref>(sc),
                                      res.message, bfa::yield);
    } catch(bs::system_error& e) {
        log_err("failed to write response: " << e.what());
        socket().close();
    }
}

}  // petrel
