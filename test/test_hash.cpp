/*
 * Copyright (c) 2016 Andreas Pohl
 * Licensed under MIT (see COPYING)
 *
 * Author: Andreas Pohl
 */

#include "builtin/hash.h"
#include "lua_state_manager.h"
#include "lua_utils.h"

#include <boost/utility/string_ref.hpp>

#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

using namespace petrel;
using namespace petrel::lib;

lua_state_manager g_le;

BOOST_AUTO_TEST_CASE(test_md5) {
    auto Lex = g_le.create_state();
    // create test function
    const std::string lua_code = "function test_md5(str) return hash.md5(str) end";
    lua_utils::load_code_from_string(Lex.L, lua_code.c_str());
    // load test function
    lua_getglobal(Lex.L, "test_md5");
    BOOST_CHECK(lua_isfunction(Lex.L, -1));
    // push function parameter
    lua_pushstring(Lex.L, "petrel roxx :-)");
    // call function
    BOOST_CHECK(!lua_pcall(Lex.L, 1, 1, Lex.traceback_idx));
    // check result
    BOOST_CHECK(lua_isstring(Lex.L, -1));
    boost::string_ref ret(lua_tostring(Lex.L, -1));
    BOOST_CHECK(ret == "d482fa70a78ca1fffbfff6e0d0d314a5");
    g_le.destroy_state(Lex);
}

BOOST_AUTO_TEST_CASE(test_sha1) {
    auto Lex = g_le.create_state();
    // create test function
    const std::string lua_code = "function test_sha1(str) return hash.sha1(str) end";
    lua_utils::load_code_from_string(Lex.L, lua_code.c_str());
    // load test function
    lua_getglobal(Lex.L, "test_sha1");
    BOOST_CHECK(lua_isfunction(Lex.L, -1));
    // push function parameter
    lua_pushstring(Lex.L, "petrel is super :-)");
    // call function
    BOOST_CHECK(!lua_pcall(Lex.L, 1, 1, Lex.traceback_idx));
    // check result
    BOOST_CHECK(lua_isstring(Lex.L, -1));
    boost::string_ref ret(lua_tostring(Lex.L, -1));
    BOOST_CHECK(ret == "c6b64747e8b1302220982c33b541981838fd28dd");
    g_le.destroy_state(Lex);
}

BOOST_AUTO_TEST_CASE(test_fnv) {
    auto Lex = g_le.create_state();
    // create test function
    const std::string lua_code = "function test_fnv(str) return hash.fnv(str) end";
    lua_utils::load_code_from_string(Lex.L, lua_code.c_str());
    // load test function
    lua_getglobal(Lex.L, "test_fnv");
    BOOST_CHECK(lua_isfunction(Lex.L, -1));
    // push function parameter
    lua_pushstring(Lex.L, "petrel yeah :-)");
    // call function
    BOOST_CHECK(!lua_pcall(Lex.L, 1, 1, Lex.traceback_idx));
    // check result
    BOOST_CHECK(lua_isnumber(Lex.L, -1));
    auto ret = static_cast<std::uint32_t>(lua_tointeger(Lex.L, -1));
    BOOST_CHECK(ret == 1012600862);
    g_le.destroy_state(Lex);
}
