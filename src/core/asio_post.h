/*
 * Copyright (c) 2016 Andreas Pohl
 * Licensed under MIT (see COPYING)
 *
 * Author: Andreas Pohl
 */

#ifndef ASIO_POST_H
#define ASIO_POST_H

#include <boost/asio.hpp>
#include <functional>

namespace petrel {

namespace ba = boost::asio;

/// Call io_service::post and wait until the handler completes.
void io_service_post_wait(ba::io_service* iosvc, std::function<void()> func);

}  // petrel

#endif  // ASIO_POST_H
