/*
 * Copyright (c) 2016 Andreas Pohl
 * Licensed under MIT (see COPYING)
 *
 * Author: Andreas Pohl
 */

#include "asio_post.h"

#include <atomic>
#include <thread>

namespace petrel {

void io_service_post_wait(ba::io_service* iosvc, std::function<void()> func) {
    std::atomic_bool done{false};
    iosvc->post([&] {
        func();
        done = true;
    });
    while (!done) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

}  // petrel
