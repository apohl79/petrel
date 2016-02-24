/*
 * Copyright (c) 2016 Andreas Pohl
 * Licensed under MIT (see COPYING)
 *
 * Author: Andreas Pohl
 */

#include "example.h"
#include <boost/utility/string_ref.hpp>

DECLARE_LIB_BEGIN(example);
ENABLE_LIB_LOAD();
ENABLE_LIB_UNLOAD();
ENABLE_LIB_STATE_INIT();
ADD_LIB_FUNCTION(work_static);
ADD_LIB_METHOD(work_member);
DECLARE_LIB_END();

int example::work_static(lua_State* L) {
    double n1 = luaL_checknumber(L, 1);
    double n2 = luaL_checknumber(L, 2);
    lua_pushnumber(L, n1 + n2);
    return 1;  // one result
}

int example::work_member(lua_State* L) {
    boost::string_ref str = luaL_checkstring(L, 1);
    lua_pushnumber(L, str.size());
    return 1;  // one result
}
