/*
 * Copyright (c) 2016 Andreas Pohl
 * Licensed under MIT (see COPYING)
 *
 * Author: Andreas Pohl
 */

#include "file_cache.h"
#include "log.h"
#include "server.h"
#include "server_impl.h"
#include "options.h"
#include "fiber_sched_algorithm.h"
#include "make_unique.h"
#include "builtin/http_client.h"

#include <fstream>
#include <boost/filesystem.hpp>
#include <boost/asio.hpp>
#include <boost/fiber/all.hpp>

#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

using namespace petrel;
using namespace petrel::lib;
using namespace boost::asio;
using namespace boost::fibers;
using namespace boost::filesystem;

BOOST_AUTO_TEST_CASE(test_cache) {
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

BOOST_AUTO_TEST_CASE(test_server) {
    //log::init();
    set_log_tag("test");

    create_directories("/tmp/petrel-test");

    std::ofstream of;
    of.open("/tmp/petrel-test/test");
    of << "test";
    of.close();

    const char* argv[] = {"test", "--server.listen=localhost", "--server.port=18588", "--lua.root=.",
                          "--lua.statebuffer=5"};
    options::parse(sizeof(argv) / sizeof(const char*), argv);
    server s;
    auto& se = s.get_lua_engine();
    se.add_lua_code_to_buffer(
        "function bootstrap() "
        "  petrel.add_directory_route(\"/files/\", \"/tmp/petrel-test\") "
        "end ");

    // start server
    s.impl()->init();
    s.impl()->start();

    // setup http2 client
    server s2;
    io_service iosvc;
    io_service::work work(iosvc);

    auto& ce = s2.get_lua_engine();
    ce.add_lua_code_to_buffer(
        "function test() "
        "  h = http_client() "
        "  h:connect(\"localhost\", \"18588\") "
        "  return h:get(\"/files/test\") "
        "end ");
    auto Lex = ce.create_lua_state();
    log_info("state created");

    std::thread([&] {
        fiber([&] {
            set_log_tag("test_clnt");
            log_info("started");

            Lex.ctx->p_server = &s2;
            Lex.ctx->p_io_service = &iosvc;
            lua_getglobal(Lex.L, "test");
            BOOST_CHECK(lua_isfunction(Lex.L, -1));
            // call function
            if (lua_pcall(Lex.L, 0, 2, Lex.traceback_idx)) {
                BOOST_CHECK_MESSAGE(false, "lua_pcall failed: " << lua_tostring(Lex.L, -1));
            } else {
                // check result
                BOOST_CHECK(lua_isstring(Lex.L, -1));
                BOOST_CHECK(lua_isinteger(Lex.L, -2));
                std::size_t content_len;
                const char* content = lua_tolstring(Lex.L, -1, &content_len);
                BOOST_CHECK(content_len == 4);
                int status = lua_tointeger(Lex.L, -2);
                BOOST_CHECK_MESSAGE(!std::strncmp(content, "test", content_len), "'test' expected: content was '"
                                                                                     << content << "'");
                BOOST_CHECK(status == 200);
            }
            log_info("get done");

            iosvc.stop();
        }).detach();

        use_scheduling_algorithm<fiber_sched_algorithm>(iosvc);
        iosvc.run();
    }).join();

    ce.destroy_lua_state(Lex);

    s.impl()->stop();
    s.impl()->join();

    remove_all("/tmp/petrel-test");
}
