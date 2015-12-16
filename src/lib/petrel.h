#ifndef LIB_PETREL_H
#define LIB_PETREL_H

#include "library.h"
#include "lua_engine.h"

namespace petrel {
namespace lib {

class petrel : public library {
  public:
    petrel(lib_context* ctx) : library(ctx) {}
    static void init(lua_State*) {}

    /// Add a route. This function takes two parameters: (1) a path and (2) a lua function name. The function will be
    /// called for each request with the given path.
    static int set_route(lua_State* L);

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
