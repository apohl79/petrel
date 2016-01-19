/*
 * Copyright (c) 2016 Andreas Pohl
 * Licensed under MIT (see COPYING)
 *
 * Author: Andreas Pohl
 */

#include <memory>
#include <ctime>
#include "counter.h"

#define BOOST_TEST_MODULE test_counter
//#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

using namespace petrel::metrics;

BOOST_AUTO_TEST_SUITE(test_counter_suite);


BOOST_AUTO_TEST_CASE(test_count) {
    counter c;
    BOOST_CHECK(c.get() == 0);

    c.increment();
    BOOST_CHECK(c.get() == 1);

    c.increment(9);
    BOOST_CHECK(c.get() == 10);

    c.decrement();
    BOOST_CHECK(c.get() == 9);

    c.decrement(4);
    BOOST_CHECK(c.get() == 5);

    auto x = c.exchange(50);
    BOOST_CHECK(c.get() == 50);
    BOOST_CHECK(x == 5);

    x = c.get_and_reset();
    BOOST_CHECK(c.get() == 0);
    BOOST_CHECK(x == 50);
}

BOOST_AUTO_TEST_SUITE_END();
