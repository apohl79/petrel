/*
 * Copyright (c) 2016 Andreas Pohl
 * Licensed under MIT (see COPYING)
 *
 * Author: Andreas Pohl
 */

#include "petrel.h"
#include "server.h"
#include "server_impl.h"
#include "log.h"
#include "make_unique.h"

#include <dlfcn.h>
#include <string>
#include <sstream>
#include <vector>
#include <memory>
#include <algorithm>
#include <boost/utility/string_ref.hpp>
#include <boost/filesystem.hpp>
#include <boost/fiber/all.hpp>

namespace petrel {
namespace lib {

DECLARE_LIB(petrel);
ENABLE_LIB_LOAD();
ADD_LIB_FUNCTION(set_route);
ADD_LIB_FUNCTION(lib_search_path);
ADD_LIB_FUNCTION(add_lib_search_path);
ADD_LIB_FUNCTION(load_lib);
ADD_LIB_FUNCTION(sleep_nanos);
ADD_LIB_FUNCTION(sleep_micros);
ADD_LIB_FUNCTION(sleep_millis);
REGISTER_LIB_BUILTIN();

/// List of paths to search for libraries loaded via load_lib
using path_list = std::vector<std::string>;
std::unique_ptr<path_list> g_lib_search_path;

void petrel::load() {
    g_lib_search_path = std::make_unique<path_list>();
    g_lib_search_path->push_back("/usr/lib");
    g_lib_search_path->push_back("/usr/local/lib");
    boost::string_ref libsuffix(PETREL_LIBDIR_SUFFIX);
    if (libsuffix.size() > 0) {
        std::ostringstream os;
        os << "/usr/lib" << libsuffix;
        g_lib_search_path->push_back(os.str());
        os.str("");
        os << "/usr/local/lib" << libsuffix;
        g_lib_search_path->push_back(os.str());
    }
    boost::string_ref instdir(PETREL_LIB_INSTALL_DIR);
    auto p = std::find(g_lib_search_path->begin(), g_lib_search_path->end(), instdir);
    if (p == g_lib_search_path->end()) {
        g_lib_search_path->push_back(PETREL_LIB_INSTALL_DIR);
    }
}

int petrel::set_route(lua_State* L) {
    std::string path = luaL_checkstring(L, 1);
    std::string func = luaL_checkstring(L, 2);
    context(L).server().impl()->set_route(path, func);
    return 0; // no results
}

int petrel::lib_search_path(lua_State* L) {
    bool first = true;
    std::ostringstream os;
    for (auto& p : *g_lib_search_path) {
        if (first) {
            first = false;
        } else {
            os << ":";
        }
        os << p;
    }
    lua_pushstring(L, os.str().c_str());
    return 1;
}

int petrel::add_lib_search_path(lua_State* L) {
    g_lib_search_path->push_back(luaL_checkstring(L, 1));
    return 0;
}

int petrel::load_lib(lua_State* L) {
    const char* lib = luaL_checkstring(L, 1);

    set_log_tag("petrel");

    std::ostringstream libname;
    for (auto& path : *g_lib_search_path) {
        libname.str("");
        libname << path << "/" << PETREL_LIB_PREFIX << lib << PETREL_LIB_SUFFIX;

        if (!boost::filesystem::exists(libname.str())) {
            continue;
        }

        void* hndl = dlopen(libname.str().c_str(), RTLD_NOW);
        if (nullptr == hndl) {
            log_err("error loading " << dlerror());
            continue;
        }

        using name_func_type = const char* (*)();
        using open_func_type = lua_CFunction;
        using init_func_type = lua_CFunction;
        using load_func_type = void (*)();

        const char* err;
        name_func_type name_func = reinterpret_cast<name_func_type>(dlsym(hndl, "petrel_lib_name"));
        if ((err = dlerror()) != nullptr) {
            log_err("error loading " << err);
            dlclose(hndl);
            continue;
        }
        open_func_type open_func = reinterpret_cast<open_func_type>(dlsym(hndl, "petrel_lib_open"));
        if ((err = dlerror()) != nullptr) {
            log_err("error loading " << err);
            dlclose(hndl);
            return 0;
        }
        init_func_type init_func = reinterpret_cast<init_func_type>(dlsym(hndl, "petrel_lib_init"));
        if ((err = dlerror()) != nullptr) {
            log_err("error loading " << err);
            dlclose(hndl);
            continue;
        }
        load_func_type load_func = reinterpret_cast<load_func_type>(dlsym(hndl, "petrel_lib_load"));
        if ((err = dlerror()) != nullptr) {
            log_err("error loading " << err);
            dlclose(hndl);
            continue;
        }
        load_func_type unload_func = reinterpret_cast<load_func_type>(dlsym(hndl, "petrel_lib_unload"));
        if ((err = dlerror()) != nullptr) {
            log_err("error loading " << err);
            dlclose(hndl);
            continue;
        }

        lua_engine::register_lib(name_func(), open_func, init_func, load_func, unload_func);

        return 0;
    }

    log_err("error loading " << lib << ": no library found");

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
