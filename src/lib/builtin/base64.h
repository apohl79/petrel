/*
 * Copyright (c) 2016 Andreas Pohl
 * Licensed under MIT (see COPYING)
 *
 * Author: Andreas Pohl
 */

#ifndef LIB_BASE64_H
#define LIB_BASE64_H

#include "library_builtin.h"

namespace petrel {
namespace lib {

class base64 : public library {
  public:
    explicit base64(lib_context* ctx) : library(ctx) {}

    /// Encode a string to base64
    static int encode(lua_State* L);

    /// Decode a string from base64
    static int decode(lua_State* L);
};

}  // lib
}  // petrel

#endif  // LIB_BASE64_H
