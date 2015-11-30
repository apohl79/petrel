#ifndef LIB_BASE64_H
#define LIB_BASE64_H

#include "library.h"
#include "lua_engine.h"

namespace petrel {
namespace lib {

class base64 : public library {
  public:
    base64(lib_context* ctx) : library(ctx) {}

    /// Encode a string to base64
    static int encode(lua_State* L);

    /// Decode a string from base64
    static int decode(lua_State* L);
};

}  // lib
}  // petrel

#endif  // LIB_BASE64_H
