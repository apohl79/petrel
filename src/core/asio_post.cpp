/*
 * Copyright (c) 2016 Andreas Pohl
 * Licensed under MIT (see COPYING)
 *
 * Author: Andreas Pohl
 */

#include "asio_post.h"

#include <condition_variable>
#include <mutex>

namespace petrel {

void io_service_post_wait(ba::io_service* iosvc, std::function<void()> func) {
    std::condition_variable cv;
    bool done = false;
    iosvc->post([&] {
        func();
        done = true;
        cv.notify_one();
    });
    std::mutex mtx;
    std::unique_lock<std::mutex> lock(mtx);
    cv.wait(lock, [&done] { return done; });
}

}  // petrel
