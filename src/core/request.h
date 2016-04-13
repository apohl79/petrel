/*
 * Copyright (c) 2016 Andreas Pohl
 * Licensed under MIT (see COPYING)
 *
 * Author: Andreas Pohl
 */

#ifndef REQUEST_H
#define REQUEST_H

#include <boost/asio.hpp>
#include <boost/utility/string_ref.hpp>
#include <memory>
#include <nghttp2/asio_http2_server.h>

#include "make_unique.h"
#include "server.h"
#include "session.h"

namespace petrel {

namespace http = boost::http;
namespace http2 = nghttp2::asio_http2;
namespace ba = boost::asio;
namespace bai = ba::ip;

/// request class
class request {
  public:
    using pointer = std::shared_ptr<request>;
    using http2_content_buffer_type = std::vector<std::uint8_t>;
    using header_type = std::pair<const std::string&, const std::string&>;

    enum class mode { HTTP, HTTP2 };

    enum class http_method { GET, POST, OTHER };

    static const std::string EMPTY;

    class header_iterator {
      public:
        explicit header_iterator(http::headers::const_iterator it) : m_mode(mode::HTTP), m_http_iter(it) {}
        explicit header_iterator(http2::header_map::const_iterator it) : m_mode(mode::HTTP2), m_http2_iter(it) {}

        header_iterator& operator++() {
            switch (m_mode) {
                case mode::HTTP:
                    ++m_http_iter;
                    break;
                case mode::HTTP2:
                    ++m_http2_iter;
                    break;
            }
            return *this;
        }

        const header_type operator*() const {
            switch (m_mode) {
                case mode::HTTP:
                    return *m_http_iter;
                case mode::HTTP2:
                    auto p = *m_http2_iter;
                    return std::make_pair(p.first, p.second.value);
            }
            throw std::runtime_error("invalid mode");
        }

      private:
        friend bool operator==(header_iterator& lhs, header_iterator& rhs);
        friend bool operator!=(header_iterator& lhs, header_iterator& rhs);
        mode m_mode;
        http::headers::const_iterator m_http_iter;
        http2::header_map::const_iterator m_http2_iter;
    };

    /// HTTP ctor.
    explicit request(session::request_type::pointer req) : m_mode(mode::HTTP), m_http_request(req) { init(); }

    /// HTTP2 ctor.
    request(const http2::server::request& req, const http2::server::response& res, server& srv,
            std::shared_ptr<http2_content_buffer_type> content)
        : m_mode(mode::HTTP2), m_http2(std::make_unique<http2_data>(srv, req, res, content)) {
        m_http2->path = req.uri().raw_path;
        if (!req.uri().raw_query.empty()) {
            m_http2->path += '?';
            m_http2->path += req.uri().raw_query;
        }
        init();
    }

    /// Dtor.
    virtual ~request() {}

    /// Return the HTTP method string
    inline const std::string& method_string() const {
        switch (m_mode) {
            case mode::HTTP:
                return m_http_request->method;
            case mode::HTTP2:
                return m_http2->request.method();
        }
        throw std::runtime_error("invalid mode");
    }

    /// Return the HTTP method
    inline http_method method() const { return m_method; }

    /// Return the HTTP protocol
    inline const std::string& proto() const {
        switch (m_mode) {
            case mode::HTTP:
                return m_http_request->proto;
            case mode::HTTP2:
                return m_http2->request.uri().scheme;
        }
        throw std::runtime_error("invalid mode");
    }

    /// Return the HTTP host
    inline const std::string& host() const {
        switch (m_mode) {
            case mode::HTTP:
                return header("host");
            case mode::HTTP2:
                return m_http2->request.uri().host;
        }
        throw std::runtime_error("invalid mode");
    }

    /// Return the HTTP request path
    inline const std::string& path() const {
        switch (m_mode) {
            case mode::HTTP:
                return m_http_request->path;
            case mode::HTTP2:
                return m_http2->path;
        }
        throw std::runtime_error("invalid mode");
    }

    /// Return the request content
    inline boost::string_ref content() const {
        switch (m_mode) {
            case mode::HTTP:
                return boost::string_ref(reinterpret_cast<const char*>(m_http_request->message.body().data()),
                                         m_http_request->message.body().size());
            case mode::HTTP2:
                if (nullptr != m_http2->content) {
                    return boost::string_ref(reinterpret_cast<const char*>(m_http2->content->data()),
                                             m_http2->content->size());
                } else {
                    return boost::string_ref();
                }
        }
        throw std::runtime_error("invalid mode");
    }

    /// Return the client endpoint
    const bai::tcp::endpoint& remote_endpoint() const {
        switch (m_mode) {
            case mode::HTTP:
                return m_http_request->remote_endpoint;
            case mode::HTTP2:
                return m_http2->request.remote_endpoint();
        }
        throw std::runtime_error("invalid mode");
    }

    /// Return a ref to the server object
    server& get_server() {
        switch (m_mode) {
            case mode::HTTP:
                return m_http_request->get_server();
            case mode::HTTP2:
                return m_http2->srv;
        }
        throw std::runtime_error("invalid mode");
    }

    ba::io_service& get_io_service() {
        switch (m_mode) {
            case mode::HTTP:
                return m_http_request->get_io_service();
            case mode::HTTP2:
                return m_http2->response.io_service();
        }
        throw std::runtime_error("invalid mode");
    }

    /// Add a response header
    void add_header(const std::string& name, const std::string& val) {
        switch (m_mode) {
            case mode::HTTP:
                m_http_request->response.message.headers().emplace(std::make_pair(name, val));
                break;
            case mode::HTTP2:
                m_http2->headers.emplace(std::make_pair(name, http2::header_value{val, false}));
                break;
        }
    }

    /// Return number of headers
    std::size_t headers_size() const {
        switch (m_mode) {
            case mode::HTTP:
                return m_http_request->message.headers().size();
            case mode::HTTP2:
                return m_http2->request.header().size();
        }
        throw std::runtime_error("invalid mode");
    }

    /// Return a header value
    inline const std::string& header(const std::string& name) const {
        switch (m_mode) {
            case mode::HTTP: {
                auto it = m_http_request->message.headers().find(name);
                if (m_http_request->message.headers().end() != it) {
                    return it->second;
                }
                return EMPTY;
            }
            case mode::HTTP2: {
                auto it = m_http2->request.header().find(name);
                if (m_http2->request.header().end() != it) {
                    return it->second.value;
                }
                return EMPTY;
            }
        }
        throw std::runtime_error("invalid mode");
    }

    /// Return true if a header exists
    bool header_exists(const std::string& name) {
        switch (m_mode) {
            case mode::HTTP:
                return m_http_request->message.headers().find(name) != m_http_request->message.headers().end();
            case mode::HTTP2:
                return m_http2->request.header().find(name) != m_http2->request.header().end();
        }
        throw std::runtime_error("invalid mode");
    }

    /// Return true if a header exists
    bool response_header_exists(const std::string& name) {
        switch (m_mode) {
            case mode::HTTP:
                return m_http_request->response.message.headers().find(name) !=
                       m_http_request->response.message.headers().end();
            case mode::HTTP2:
                return m_http2->headers.find(name) != m_http2->headers.end();
        }
        throw std::runtime_error("invalid mode");
    }

    /// Return a begin iterator to walk through the headers
    const header_iterator headers_begin() const {
        switch (m_mode) {
            case mode::HTTP:
                return header_iterator(m_http_request->message.headers().begin());
            case mode::HTTP2:
                return header_iterator(m_http2->request.header().begin());
        }
        throw std::runtime_error("invalid mode");
    }

    /// Return an end iterator
    const header_iterator headers_end() const {
        switch (m_mode) {
            case mode::HTTP:
                return header_iterator(m_http_request->message.headers().end());
            case mode::HTTP2:
                return header_iterator(m_http2->request.header().end());
        }
        throw std::runtime_error("invalid mode");
    }

    /// Send an error
    void send_error_response(int code) { send_response(code, boost::string_ref()); }

    // Send response
    void send_response(int code, boost::string_ref content) {
        if (!response_header_exists("server")) {
            add_header("server", "petrel");
        }
        switch (m_mode) {
            case mode::HTTP:
                if (content.size() > 0) {
                    std::copy(content.begin(), content.end(),
                              std::back_inserter(m_http_request->response.message.body()));
                }
                m_http_request->response.status = code;
                m_http_request->send_response();
                break;
            case mode::HTTP2:
                std::string data(content.data(), content.size());
                add_header("content-length", std::to_string(content.size()));
                m_http2->response.write_head(code, std::move(m_http2->headers));
                m_http2->response.end(std::move(data));
                break;
        }
    }

  private:
    mode m_mode;
    http_method m_method{http_method::OTHER};

    // http1
    session::request_type::pointer m_http_request;

    // http2
    struct http2_data {
        http2_data(server& s, const http2::server::request& req, const http2::server::response& res,
                   std::shared_ptr<http2_content_buffer_type> cnt)
            : srv(s), request(req), response(res), content(cnt) {}
        server& srv;
        const http2::server::request& request;
        const http2::server::response& response;
        http2::header_map headers;
        std::shared_ptr<http2_content_buffer_type> content;
        std::string path;
    };
    std::unique_ptr<http2_data> m_http2;

    /// Initialize general members
    void init() {
        auto& method = method_string();
        if (method == "GET") {
            m_method = http_method::GET;
        } else if (method == "POST") {
            m_method = http_method::POST;
        }
    }
};

bool operator==(request::header_iterator& lhs, request::header_iterator& rhs);
bool operator!=(request::header_iterator& lhs, request::header_iterator& rhs);

}  // petrel

#endif  // REQUEST_H
