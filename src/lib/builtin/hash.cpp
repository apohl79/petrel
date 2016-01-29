/*
 * Copyright (c) 2016 Andreas Pohl
 * Licensed under MIT (see COPYING)
 *
 * Author: Andreas Pohl
 */

#include "hash.h"

#include <memory>
#include <vector>
#include <array>
#include <functional>
#include <sstream>
#include <iomanip>
#include <boost/utility/string_ref.hpp>
#include <openssl/evp.h>

namespace petrel {
namespace lib {

DECLARE_LIB_BEGIN(hash);
ADD_LIB_FUNCTION(md5);
ADD_LIB_FUNCTION(sha1);
ADD_LIB_FUNCTION(fnv);
DECLARE_LIB_BUILTIN_END();

using evp_hash_type = std::array<std::uint8_t, EVP_MAX_MD_SIZE>;

std::uint32_t evp_hash(lua_State* L, const EVP_MD* md, evp_hash_type& result, const char* data, std::size_t len) {
    std::unique_ptr<EVP_MD_CTX, std::function<void(EVP_MD_CTX*)>> md_ctx(EVP_MD_CTX_create(), EVP_MD_CTX_destroy);
    if (nullptr == md_ctx) {
        luaL_error(L, "failed to create md_ctx");
    }
    if (!EVP_DigestInit_ex(md_ctx.get(), md, 0)) {
        luaL_error(L, "failed to initialize digest");
    }
    if (!EVP_DigestUpdate(md_ctx.get(), data, len)) {
        luaL_error(L, "failed to update digest");
    }
    std::uint32_t size = 0;
    if (!EVP_DigestFinal_ex(md_ctx.get(), result.data(), &size)) {
        luaL_error(L, "failed to finalize digest");
    }
    return size;
}

void evp_pushhash(lua_State* L, evp_hash_type& h, std::uint32_t size) {
    std::ostringstream os;
    os << std::hex;
    for (std::uint32_t i = 0; i < size; ++i) {
        os << std::setw(2) << std::setfill('0') << static_cast<std::uint16_t>(h[i]);
    }
    lua_pushstring(L, os.str().c_str());
}

int hash::md5(lua_State* L) {
    boost::string_ref str(luaL_checkstring(L, 1));
    const EVP_MD *md = EVP_md5();
    if (nullptr == md) {
        luaL_error(L, "failed to get sha1 md");
    }
    evp_hash_type h;
    evp_pushhash(L, h, evp_hash(L, md, h, str.data(), str.size()));
    return 1;
}

int hash::sha1(lua_State* L) {
    boost::string_ref str(luaL_checkstring(L, 1));
    const EVP_MD *md = EVP_sha1();
    if (nullptr == md) {
        luaL_error(L, "failed to get sha1 md");
    }
    evp_hash_type h;
    evp_pushhash(L, h, evp_hash(L, md, h, str.data(), str.size()));
    return 1;
}

int hash::fnv(lua_State* L) {
    boost::string_ref str(luaL_checkstring(L, 1));
    auto* p = reinterpret_cast<const unsigned char*>(str.data());
    std::uint32_t h = 2166136261;
    for (std::size_t i = 0; i < str.size(); ++i) {
        h = (h * 16777619) ^ p[i];
    }
    lua_pushinteger(L, h);
    return 1;
}

}  // lib
}  // petrel
