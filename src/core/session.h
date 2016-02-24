/*
 * Copyright (c) 2016 Andreas Pohl
 * Licensed under MIT (see COPYING)
 *
 * Author: Andreas Pohl
 */

#ifndef SESSION_H
#define SESSION_H

#include <memory>
#include <queue>

#include <boost/asio.hpp>
#include <boost/fiber/all.hpp>

#include "boost/http/buffered_socket.hpp"
#include "boost/http/status_code.hpp"
#include "log.h"

namespace petrel {

namespace ba = boost::asio;
namespace bai = boost::asio::ip;
namespace http = boost::http;
namespace bf = boost::fibers;

class server;

/// Session class
/// Each new client gets handled by a session object
class session : public std::enable_shared_from_this<session> {
  public:
    set_log_tag_default_priority("session");

    using pointer = std::shared_ptr<session>;

    struct response_t {
        response_t(const response_t&) = delete;
        response_t(response_t&&) = default;
        response_t() : status(200) {}
        explicit response_t(std::uint_fast16_t s) : status(s) {}
        response_t& operator=(const response_t&) = delete;
        response_t& operator=(response_t&&) = default;
        std::uint_fast16_t status;
        http::message message;
    };

    using response_type = response_t;

    class request {
      public:
        using pointer = std::shared_ptr<request>;

        explicit request(session::pointer s) : m_session(s), m_res_future(m_res_promise.get_future()) {}
        request(const request&) = delete;
        request(request&&) = default;
        request& operator=(const request&) = delete;
        request& operator=(request&&) = default;

        server& get_server() { return m_session->get_server(); }
        ba::io_service& get_io_service() { return m_session->get_io_service(); }

        /// Send an error response with the given error code
        void send_error_response(std::uint_fast16_t status) {
            response.status = status;
            send_response();
        }

        /// Send a response
        void send_response(std::uint_fast16_t status, http::message msg) {
            response.status = status;
            response.message = std::move(msg);
            send_response();
        }

        void send_response() { m_res_promise.set_value(); }

        void wait() { m_res_future.get(); }

        std::string method;
        std::string path;
        std::string proto = "http";
        http::message message;
        bai::tcp::endpoint remote_endpoint;
        response_type response;

      private:
        session::pointer m_session;
        bf::promise<void> m_res_promise;
        bf::future<void> m_res_future;
    };

    using request_type = request;

    /// Ctor.
    session(server& srv, ba::io_service& iosvc);

    /// Dtor.
    virtual ~session();

    /// Return the sessions underlying socket object.
    bai::tcp::socket& socket() { return m_socket.next_layer(); }

    /// Return the server
    server& get_server();

    /// Return the io service
    ba::io_service& get_io_service();

    /// Start a session for a new client.
    void start();

    /// Send a response
    void send_response(response_type& res);

  private:
    server& m_srv;
    ba::io_service& m_iosvc;
    http::buffered_socket m_socket;
};

}  // petrel

#endif  // SESSION_H
