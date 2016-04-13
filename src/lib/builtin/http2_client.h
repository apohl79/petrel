/*
 * Copyright (c) 2016 Andreas Pohl
 * Licensed under MIT (see COPYING)
 *
 * Author: Andreas Pohl
 */

#ifndef LIB_HTTP2_CLIENT_H
#define LIB_HTTP2_CLIENT_H

#include <memory>
#include <nghttp2/asio_http2_client.h>

#include "library_builtin.h"

namespace petrel {
namespace lib {

namespace ba = boost::asio;
namespace bp = boost::posix_time;
namespace http2 = nghttp2::asio_http2;

/// http2_client class
class http2_client : public library {
  public:
    explicit http2_client(lib_context* ctx) : library(ctx), m_read_timeout(0, 0, 5, 0) {}

    int connect(lua_State* L);
    int disconnect(lua_State* L);
    int get(lua_State* L);
    int read_timeout(lua_State* L);

  private:
    std::unique_ptr<http2::client::session> m_session;
    std::string m_host;
    std::string m_port;
    bool m_ssl = false;
    bool m_ssl_verify = true;
    std::unique_ptr<ba::ssl::context> m_tls;
    bp::time_duration m_read_timeout;
    bool m_connected = false;

    inline void close() {
        if (m_connected) {
            m_session->shutdown();
            m_session.reset(nullptr);
            m_connected = false;
        }
    }
};

}  // lib
}  // petrel

#endif  // LIB_HTTP2_CLIENT_H
