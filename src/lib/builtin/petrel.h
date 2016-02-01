/*
 * Copyright (c) 2016 Andreas Pohl
 * Licensed under MIT (see COPYING)
 *
 * Author: Andreas Pohl
 */

#ifndef LIB_PETREL_H
#define LIB_PETREL_H

#include "lua_engine.h"
#include "library.h"

namespace petrel {
namespace lib {

class petrel : public library {
  public:
    petrel(lib_context* ctx) : library(ctx) {}
    static void load();

    /// Add a route. This function takes two parameters: (1) a path and (2) a lua function name. The function will be
    /// called for each request with the given path.
    static int add_route(lua_State* L);

    /// Return the library search path list
    static int lib_search_path(lua_State* L);

    /// Add a path to the library search path list
    static int add_lib_search_path(lua_State* L);

    /// Load a library shared object file
    static int load_lib(lua_State* L);

    /// Sleep for N nanoseconds
    static int sleep_nanos(lua_State* L);

    /// Sleep for N microseconds
    static int sleep_micros(lua_State* L);

    /// Sleep for N milliseconds
    static int sleep_millis(lua_State* L);
};

}  // lib
}  // petrel

#endif  // LIB_PETREL_H
