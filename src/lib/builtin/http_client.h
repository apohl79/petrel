/*
 * Copyright (c) 2016 Andreas Pohl
 * Licensed under MIT (see COPYING)
 *
 * Author: Andreas Pohl
 */

#ifndef LIB_HTTP_CLIENT_H
#define LIB_HTTP_CLIENT_H

#include <boost/asio.hpp>

#include "library_builtin.h"

namespace petrel {
namespace lib {

namespace ba = boost::asio;
namespace bp = boost::posix_time;
namespace bs = boost::system;

class http_client : public library {
  public:
    explicit http_client(lib_context* ctx)
        : library(ctx), m_sock(m_iosvc), m_read_timeout(0, 0, 5, 0), m_connect_timeout(0, 0, 5, 0) {}

    int connect(lua_State* L);
    int disconnect(lua_State* L);
    int get(lua_State* L);
    int post(lua_State* L);
    int read_timeout(lua_State* L);
    int connect_timeout(lua_State* L);
    int disable_keepalive(lua_State* L);

  private:
    ba::ip::tcp::socket m_sock;
    ba::streambuf m_req_buf;
    ba::streambuf m_res_buf;
    std::string m_host;
    std::string m_port;
    bp::time_duration m_read_timeout;
    bp::time_duration m_connect_timeout;
    bool m_keep_alive = true;
    bool m_connected = false;

    int send_recv(lua_State* L);

    void timeout_handler(const bs::error_code& ec);

    inline void close() {
        if (m_connected) {
            m_sock.close();
            m_connected = false;
        }
    }
};

}  // lib
}  // petrel

#endif  // LIB_HTTP_CLIENT_H
