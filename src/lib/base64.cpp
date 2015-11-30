#include "base64.h"

#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/utility/string_ref.hpp>

namespace petrel {
namespace lib {

REGISTER_LIB(base64);
REGISTER_LIB_FUNCTION(base64, encode);
REGISTER_LIB_FUNCTION(base64, decode);

using namespace boost::archive::iterators;
using namespace boost::algorithm;

int base64::encode(lua_State* L) {
    boost::string_ref str = luaL_checkstring(L, 1);
    using it_type = base64_from_binary<transform_width<std::string::const_iterator, 6, 8>>;
    auto out = std::string(it_type(str.begin()), it_type(str.end()));
    out.append((3 - str.size() % 3) % 3, '=');
    lua_pushstring(L, out.c_str());
    return 1;
}

int base64::decode(lua_State* L) {
    boost::string_ref str = luaL_checkstring(L, 1);
    using it_type = transform_width<binary_from_base64<std::string::const_iterator>, 8, 6>;
    auto out =
        trim_right_copy_if(std::string(it_type(str.begin()), it_type(str.end())), [](char c) { return c == '\0'; });
    lua_pushstring(L, out.c_str());
    return 1;
}

}  // lib
}  // petrel
