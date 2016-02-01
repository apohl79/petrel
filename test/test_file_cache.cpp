/*
 * Copyright (c) 2016 Andreas Pohl
 * Licensed under MIT (see COPYING)
 *
 * Author: Andreas Pohl
 */

#include "file_cache.h"
#include "log.h"

#include <fstream>
#include <boost/filesystem.hpp>

#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

using namespace petrel;
using namespace boost::filesystem;

BOOST_AUTO_TEST_CASE(test_file) {
    //log::init();
    set_log_tag("test");

    create_directories("/tmp/petrel-test/1/2");

    std::ofstream of;
    of.open("/tmp/petrel-test/test1");
    of << "test";
    of.close();
    of.open("/tmp/petrel-test/1/test2");
    of << "test2_";
    of.close();
    of.open("/tmp/petrel-test/1/2/test3");
    of << "test3__";
    of.close();

    file_cache::file f1("/tmp/petrel-test/test1");

    BOOST_CHECK(f1 == file_cache::file("/tmp/petrel-test/test1"));

    of.open("/tmp/petrel-test/test1");
    of << "test1";
    of.close();

    BOOST_CHECK(f1 != file_cache::file("/tmp/petrel-test/test1"));

    file_cache cache(1);
    cache.add_directory("/tmp/petrel-test");

    auto ptr = cache.get_file("/tmp/petrel-test/test1");
    BOOST_CHECK(ptr != nullptr);
    BOOST_CHECK(ptr->size() == 5);
    BOOST_CHECK(!strncmp(ptr->data().data(), "test1", 5));
    ptr = cache.get_file("/tmp/petrel-test/1/test2");
    BOOST_CHECK(ptr != nullptr);
    BOOST_CHECK(ptr->size() == 6);
    BOOST_CHECK(!strncmp(ptr->data().data(), "test2_", 6));
    ptr = cache.get_file("/tmp/petrel-test/1/2/test3");
    BOOST_CHECK(ptr != nullptr);
    BOOST_CHECK(ptr->size() == 7);
    BOOST_CHECK(!strncmp(ptr->data().data(), "test3__", 7));
    BOOST_CHECK(nullptr == cache.get_file("/tmp/petrel-test/testX"));

    of.open("/tmp/petrel-test/test1");
    of << "test1____";
    of.close();

    of.open("/tmp/petrel-test/1/test5");
    of << "test5___";
    of.close();

    std::this_thread::sleep_for(std::chrono::seconds(2));

    ptr = cache.get_file("/tmp/petrel-test/test1");
    BOOST_CHECK(ptr != nullptr);
    BOOST_CHECK(ptr->size() == 9);
    BOOST_CHECK(!strncmp(ptr->data().data(), "test1____", 9));
    ptr = cache.get_file("/tmp/petrel-test/1/test5");
    BOOST_CHECK(ptr != nullptr);
    BOOST_CHECK(ptr->size() == 8);
    BOOST_CHECK(!strncmp(ptr->data().data(), "test5___", 8));

    remove("/tmp/petrel-test/test1");

    std::this_thread::sleep_for(std::chrono::seconds(2));

    ptr = cache.get_file("/tmp/petrel-test/test1");
    BOOST_CHECK(ptr == nullptr);

    remove_all("/tmp/petrel-test");
}
