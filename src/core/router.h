/*
 * Copyright (c) 2016 Andreas Pohl
 * Licensed under MIT (see COPYING)
 *
 * Author: Andreas Pohl
 */

#ifndef ROUTER_H
#define ROUTER_H

#include <algorithm>
#include <functional>

#include <nghttp2/asio_http2_server.h>

#include "boost/http/message.hpp"
#include "boost/http/status_code.hpp"
#include "request.h"

namespace petrel {

namespace http = boost::http;
namespace http2 = nghttp2::asio_http2;

/// A set like struct that is indexing a string by character
template <typename Node>
class path_set {
  public:
    using node_type = Node;
    using func_type = typename node_type::func_type;

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

/// A router maps path strings to handler functions.
class router {
  public:
    using route_func_type = std::function<void(request::pointer)>;

    router() {
        // 404 default
        m_default_func = [](request::pointer req) { req->send_error_response(404); };
    }

    /// Add a route function for a path. All incoming requests starting with the given path string will be handled by
    /// the given function.
    void add_route(const std::string& path, route_func_type func) { m_set.insert(path, std::move(func)); }

    /// Find a route function for a path. A default function will be returned if no function can be found.
    route_func_type& find_route(const std::string& path) {
        try {
            return m_set.find(path);
        } catch (std::runtime_error&) {
            return m_default_func;
        }
    }

  private:
    path_set<path_node<route_func_type>> m_set;
    route_func_type m_default_func;
};

}  // petrel

#endif  // ROUTER_H
