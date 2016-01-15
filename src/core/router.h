/*
 * Copyright (c) 2016 Andreas Pohl
 * Licensed under MIT (see COPYING)
 *
 * Author: Andreas Pohl
 */

#ifndef ROUTER_H
#define ROUTER_H

#include <functional>
#include <algorithm>

#include <nghttp2/asio_http2_server.h>

#include "session.h"
#include "boost/http/message.hpp"
#include "boost/http/status_code.hpp"

namespace petrel {

namespace http = boost::http;
namespace http2 = nghttp2::asio_http2;

/// A router maps path strings to handler functions.
class router {
  public:
    using route_func_http_type = std::function<void(session::request_type::pointer)>;
    using route_func_http2_type =
        std::function<void(const http2::server::request& req, const http2::server::response& res)>;

    router() {
        // setup 404 defaults
        m_http_default = [](session::request_type::pointer req) { req->send_error_response(404); };
        m_http2_default = [](const http2::server::request&, const http2::server::response& res) {
            res.write_head(404);
            res.end();
        };
    }

    /// Add a route function for a path. All incoming requests starting with the given path string will be handled by
    /// the given function.
    void set_route(const std::string& path, route_func_http_type func) { m_set_http.insert(path, std::move(func)); }
    void set_route(const std::string& path, route_func_http2_type func) { m_set_http2.insert(path, std::move(func)); }

    /// Find a route function for a path for http. A default function will be returned if no function can be found.
    route_func_http_type& find_route_http(const std::string& path) {
        try {
            return m_set_http.find(path);
        } catch (std::runtime_error&) {
            return m_http_default;
        }
    }

    /// Find a route function for a path for http2. A default function will be returned if no function can be found.
    route_func_http2_type& find_route_http2(const std::string& path) {
        try {
            return m_set_http2.find(path);
        } catch (std::runtime_error&) {
            return m_http2_default;
        }
    }

  private:
    /// A node in the search set.
    template <typename Func>
    struct path_node {
        using func_type = Func;
        bool term = false;
        func_type func;
        path_node* next = nullptr;
        ~path_node() {
            if (nullptr != next) {
                delete[] next;
            }
        }
    };

    /// A set like struct that is indexing a string by character
    template <typename Node>
    class path_set {
      public:
        using node_type = Node;
        using func_type = typename node_type::func_type;

        path_set() {}

        node_type& insert(const std::string& path, func_type func) {
            auto* next = root;
            for (std::size_t i = 0; i < path.length(); ++i) {
                std::uint8_t c = path[i];
                auto& node = next[c];
                if (i == path.length() - 1) {
                    node.term = true;
                    node.func = func;
                    return node;
                } else {
                    if (nullptr == node.next) {
                        node.next = new node_type[256];
                    }
                    next = node.next;
                }
            }
            throw std::runtime_error("insert failed");
        }

        func_type& find(const std::string& path) {
            auto* next = root;
            node_type* found = nullptr;
            for (std::size_t i = 0; i < path.length(); ++i) {
                std::uint8_t c = path[i];
                auto& node = next[c];
                if (node.term) {
                    found = &node;
                }
                if (nullptr == node.next) {
                    break;
                }
                next = node.next;
            }
            if (nullptr != found) {
                return found->func;
            }
            throw std::runtime_error("find failed");
        }

      private:
        node_type root[256];
    };

    path_set<path_node<route_func_http_type>> m_set_http;
    path_set<path_node<route_func_http2_type>> m_set_http2;

    route_func_http_type m_http_default;
    route_func_http2_type m_http2_default;
};

}  // petrel

#endif  // ROUTER_H
