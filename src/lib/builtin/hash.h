/*
 * Copyright (c) 2016 Andreas Pohl
 * Licensed under MIT (see COPYING)
 *
 * Author: Andreas Pohl
 */

#ifndef LIB_HASH_H
#define LIB_HASH_H

#include "lua_engine.h"
#include "library.h"

namespace petrel {
namespace lib {

/// hash class
class hash : public library {
  public:
    hash(lib_context* ctx) : library(ctx) {}

    static int md5(lua_State* L);
    static int sha1(lua_State* L);
    static int fnv(lua_State* L);
};

}  // lib
}  // petrel

#endif  // LIB_HASH_H
