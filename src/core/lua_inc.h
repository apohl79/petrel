/*
 * Copyright (c) 2016 Andreas Pohl
 * Licensed under MIT (see COPYING)
 *
 * Author: Andreas Pohl
 */

#ifndef STOCK_LUA
// LuaJit does not need to be compiled in C++ mode so we include lua's hpp header.
#include <lua.hpp>
#else
// Stock lua needs to be compiled in C++ mode, so we need no extern "C".
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#endif
