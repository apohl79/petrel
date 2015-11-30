#include "petrel.h"
#include "server.h"

#include <boost/fiber/all.hpp>

namespace petrel {
namespace lib {

REGISTER_LIB(petrel);
REGISTER_LIB_FUNCTION(petrel, set_route);
REGISTER_LIB_FUNCTION(petrel, sleep_nanos);
REGISTER_LIB_FUNCTION(petrel, sleep_micros);
REGISTER_LIB_FUNCTION(petrel, sleep_millis);

int petrel::set_route(lua_State* L) {
    std::string path = luaL_checkstring(L, 1);
    std::string func = luaL_checkstring(L, 2);
    context(L).server().set_route(path, func);
    return 0; // no results
}

int petrel::sleep_nanos(lua_State* L) {
    int t = luaL_checkinteger(L, 1);
    boost::this_fiber::sleep_for(std::chrono::nanoseconds(t));
    return 0;
}

int petrel::sleep_micros(lua_State* L) {
    int t = luaL_checkinteger(L, 1);
    boost::this_fiber::sleep_for(std::chrono::microseconds(t));
    return 0;
}

int petrel::sleep_millis(lua_State* L) {
    int t = luaL_checkinteger(L, 1);
    boost::this_fiber::sleep_for(std::chrono::milliseconds(t));
    return 0;
}

}  // lib
}  // petrel
