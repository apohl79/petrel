/*
 * Copyright (c) 2016 Andreas Pohl
 * Licensed under MIT (see COPYING)
 *
 * Author: Andreas Pohl
 */

#ifndef LUA_UTILS_H
#define LUA_UTILS_H

#include <string>
#include <vector>

#include "log.h"
#include "lua_inc.h"

namespace petrel {

class lua_utils {
    decl_log_static();

  public:
    /// Load a lua script into the code buffer
    static void load_code_from_file(lua_State* L, const std::string& script);

    /// Load lua code from the string into the given lua state
    static void load_code_from_string(lua_State* L, const std::string& code);

    /// Load lua code from a vector of strings into the given lua state
    static void load_code_from_strings(lua_State* L, const std::vector<std::string>& codes);

    /// Find scripts in a directory and it's sub directories
    static void load_script_dir(const std::string& dir, std::vector<std::string>& vec);

    /// Load lua code from the list of scripts into the given lua state
    static void load_code_from_scripts(lua_State* L, const std::vector<std::string>& scripts);

    /// Dump the stack of a lua state
    static void dump_stack(lua_State* L);

    /// Print a lua type at the given stack index
    static void print_type(lua_State* L, int i, log_priority prio, bool show_type_name = true,
                           bool show_table_content = true, int max_table_depth = -1);

    /// Print a lua type at the given stack index (simple version)
    static void print_type_simple(lua_State* L, int i, log_priority prio, bool string_quotes = false,
                                  bool show_table = false, int max_table_depth = -1);

    /// Print a lua table at the given stack index
    static void print_table(lua_State* L, int i, log_priority prio, int level = 0, int max_level = -1);

    /// Replacement for lua's print
    static int print(lua_State* L);

    /// Traceback function
    static int traceback(lua_State* L);

    /// LUA 5.1 has no luaL_requiref
    static void lua_requiref(lua_State* L, const char* modname, lua_CFunction openf, int glb);
};

}  // petrel

#endif  // LUA_UTILS_H
