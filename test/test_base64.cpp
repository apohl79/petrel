/*
 * Copyright (c) 2016 Andreas Pohl
 * Licensed under MIT (see COPYING)
 *
 * Author: Andreas Pohl
 */

#include "lua_engine.h"
#include "builtin/hash.h"

#include <boost/utility/string_ref.hpp>

#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

using namespace petrel;
using namespace petrel::lib;

lua_engine g_le;

BOOST_AUTO_TEST_CASE(test_encode) {
    auto Lex = g_le.create_lua_state();
    // create test function
    const std::string lua_code = "function test(str) return base64.encode(str) end";
    lua_engine::load_code_from_string(Lex.L, lua_code.c_str());
    // load test function
    lua_getglobal(Lex.L, "test");
    BOOST_CHECK(lua_isfunction(Lex.L, -1));
    // push function parameter
    lua_pushstring(Lex.L, "petrel roxx :-)");
    // call function
    BOOST_CHECK(!lua_pcall(Lex.L, 1, 1, Lex.traceback_idx));
    // check result
    BOOST_CHECK(lua_isstring(Lex.L, -1));
    boost::string_ref ret(lua_tostring(Lex.L, -1));
    BOOST_CHECK(ret == "cGV0cmVsIHJveHggOi0p");
    g_le.destroy_lua_state(Lex);
}

BOOST_AUTO_TEST_CASE(test_decode) {
    auto Lex = g_le.create_lua_state();
    // create test function
    const std::string lua_code = "function test(str) return base64.decode(str) end";
    lua_engine::load_code_from_string(Lex.L, lua_code.c_str());
    // load test function
    lua_getglobal(Lex.L, "test");
    BOOST_CHECK(lua_isfunction(Lex.L, -1));
    // push function parameter
    lua_pushstring(Lex.L, "cGV0cmVsIHJveHggOi0p");
    // call function
    BOOST_CHECK(!lua_pcall(Lex.L, 1, 1, Lex.traceback_idx));
    // check result
    BOOST_CHECK(lua_isstring(Lex.L, -1));
    boost::string_ref ret(lua_tostring(Lex.L, -1));
    BOOST_CHECK(ret == "petrel roxx :-)");
    g_le.destroy_lua_state(Lex);
}
