/*
 * Copyright (c) 2016 Andreas Pohl
 * Licensed under MIT (see COPYING)
 *
 * Author: Andreas Pohl
 */

#include "builtin/http2_client.h"
#include "fiber_sched_algorithm.h"
#include "options.h"
#include "server.h"
#include "server_impl.h"

#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

#include <boost/asio.hpp>
#include <boost/fiber/all.hpp>

using namespace petrel;
using namespace petrel::lib;
using namespace boost::asio;
using namespace boost::fibers;

BOOST_AUTO_TEST_CASE(test_http2) {
    // options
    const char* argv[] = {"test", "--server.listen=localhost", "--server.port=18586", "--lua.root=.",
                          "--lua.statebuffer=5"};
    options::parse(sizeof(argv) / sizeof(const char*), argv);

    // log::init();

    // create server and push some lua code
    server s;
    auto& se = s.get_lua_engine().state_manager();
    se.add_lua_code(
        "function bootstrap() "
        "  petrel.add_route(\"/\", \"handler\") "
        "end "
        "function handler(req, res) "
        "  res.content = \"test\" "
        "  res.headers[\"x-hdr-test\"] = \"hdr-val\" "
        "  return res "
        "end");

    // start server
    s.impl()->init();
    s.impl()->start();

    set_log_tag("test_main");
    log_info("server up");

    // setup http2 client
    server s2;
    io_service iosvc;
    io_service::work work(iosvc);

    auto& ce = s2.get_lua_engine().state_manager();
    ce.add_lua_code(
        "function test() "
        "  h = http2_client() "
        "  h:connect(\"localhost\", \"18586\") "
        "  return h:get(\"/\") "
        "end");
    auto Lex = ce.create_state();
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
                BOOST_CHECK(lua_isnumber(Lex.L, -2));
                std::string content(lua_tostring(Lex.L, -1));
                int status = lua_tointeger(Lex.L, -2);
                BOOST_CHECK_MESSAGE(content == "test", "'test' expected: content was '" << content << "'");
                BOOST_CHECK(status == 200);
            }
            iosvc.stop();
            log_info("done");
        }).detach();

        use_scheduling_algorithm<fiber_sched_algorithm>(iosvc);
        iosvc.run();
    }).join();

    ce.destroy_state(Lex);

    s.impl()->stop();
    s.impl()->join();
    log_info("test done");
}
