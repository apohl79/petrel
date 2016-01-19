/*
 * Copyright (c) 2016 Andreas Pohl
 * Licensed under MIT (see COPYING)
 *
 * Author: Andreas Pohl
 */

#include "router.h"

#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

using namespace petrel;

void find_and_exec(router& r, const std::string& p) {
    auto& f = r.find_route_http(p);
    f(nullptr);
}

BOOST_AUTO_TEST_CASE(test_set_find) {
    router r;
    bool r1 = false;
    r.set_route("/", [&r1](session::request_type::pointer) { r1 = true; });
    find_and_exec(r, "/");
    BOOST_CHECK_MESSAGE(r1, "/ route not found");

    bool r2 = false;
    r.set_route("/test1", [&r2](session::request_type::pointer) { r2 = true; });
    find_and_exec(r, "/test1");
    BOOST_CHECK_MESSAGE(r2, "/test1 route not found");

    bool r3 = false;
    r.set_route("/test1/test2", [&r3](session::request_type::pointer) { r3 = true; });
    find_and_exec(r, "/test1/test2");
    BOOST_CHECK_MESSAGE(r3, "/test1/test2 route not found");

    r1 = r2 = r3 = false;

    find_and_exec(r, "/test1/test2/abc");
    BOOST_CHECK_MESSAGE(r3, "/test1/test2/abc not mapped to route /test1/test2");

    find_and_exec(r, "/test1/xxx");
    BOOST_CHECK_MESSAGE(r2, "/test1/xxx not mapped to route /test1");

    find_and_exec(r, "/xxx");
    BOOST_CHECK_MESSAGE(r1, "/xxx not mapped to route /");
}
