/*
 * Copyright (c) 2016 Andreas Pohl
 * Licensed under MIT (see COPYING)
 *
 * Author: Andreas Pohl
 */

#include "lua_utils.h"

#include <boost/filesystem.hpp>

namespace petrel {

// Initialize log members
init_log_static(lua_utils, log_priority::info);

void lua_utils::load_script_dir(const std::string& dir, std::vector<std::string>& vec) {
    if (dir.length() == 0) {
        return;
    }
    using namespace boost::filesystem;
    log_info("loading lua directory: " << dir);
    path p(dir);
    std::vector<std::string> subdirs;
    try {
        for (directory_entry& entry : directory_iterator(p)) {
            const std::string& s = entry.path().string();
            if (is_directory(entry.status())) {
                subdirs.push_back(s);
            } else {
                if (s.substr(s.size() - 4) == ".lua") {
                    log_info("  found " << s);
                    vec.push_back(s);
                }
            }
        }
    } catch (filesystem_error& e) {
        throw std::runtime_error(e.what());
    }
    for (auto& subdir : subdirs) {
        load_script_dir(subdir, vec);
    }
}

void lua_utils::load_code_from_file(lua_State* L, const std::string& script) {
    if (luaL_dofile(L, script.c_str())) {
        throw std::runtime_error(lua_tostring(L, -1));
    }
}

void lua_utils::load_code_from_string(lua_State* L, const std::string& code) {
    if (luaL_dostring(L, code.c_str())) {
        throw std::runtime_error(lua_tostring(L, -1));
    }
}

void lua_utils::load_code_from_strings(lua_State* L, const std::vector<std::string>& codes) {
    for (auto& c : codes) {
        load_code_from_string(L, c);
    }
}

void lua_utils::load_code_from_scripts(lua_State* L, const std::vector<std::string>& scripts) {
    for (auto& s : scripts) {
        lua_utils::load_code_from_file(L, s);
    }
}

void lua_utils::print_type(lua_State* L, int i, log_priority prio, bool show_type_name, bool show_table_content,
                           int max_table_depth) {
    set_log_priority(prio);
    if (lua_type(L, i) == LUA_TSTRING || lua_type(L, i) == LUA_TNUMBER) {
        print_type_simple(L, i, prio);
    } else if (lua_iscfunction(L, i) || lua_islightuserdata(L, i) || lua_isuserdata(L, i) || lua_isthread(L, i)) {
        if (show_type_name) {
            log_plain_noln(lua_typename(L, lua_type(L, i)) << ": ");
        }
        log_plain_noln(lua_topointer(L, i));
    } else if (lua_istable(L, i)) {
        if (show_type_name) {
            log_plain_noln(lua_typename(L, lua_type(L, i)) << ": ");
        }
        log_plain_noln(lua_topointer(L, i));
        if (show_table_content) {
            log_plain_noln(": ");
            print_table(L, i, prio, 0, max_table_depth);
        }
    } else {
        if (show_type_name) {
            log_plain_noln(lua_typename(L, lua_type(L, i)) << ": ");
        }
        log_plain_noln(lua_topointer(L, i));
    }
}

void lua_utils::print_type_simple(lua_State* L, int i, log_priority prio, bool string_quotes, bool show_table,
                                  int max_table_depth) {
    set_log_priority(prio);
    if (lua_type(L, i) == LUA_TSTRING) {
        if (string_quotes) {
            log_plain_noln("\"");
        }
        log_plain_noln(lua_tostring(L, i));
        if (string_quotes) {
            log_plain_noln("\"");
        }
    } else if (lua_type(L, i) == LUA_TNUMBER) {
        log_plain_noln(lua_tonumber(L, i));
    } else if (lua_type(L, i) == LUA_TTABLE) {
        if (show_table) {
            print_table(L, i, prio, 0, max_table_depth);
        } else {
            log_plain_noln("table...");
        }
    } else {
        log_plain_noln(lua_typename(L, lua_type(L, i)));
    }
}

void lua_utils::print_table(lua_State* L, int i, log_priority prio, int level, int max_level) {
    set_log_priority(prio);
    log_plain_noln(log_color::yellow << "{" << log_color::reset);
    int t = i < 0 ? lua_gettop(L) + i + 1 : i;
    bool first = true;
    lua_pushnil(L);
    while (lua_next(L, t) != 0) {
        if (first) {
            first = false;
        } else {
            log_plain_noln(", ");
        }
        log_plain_noln(log_color::white << "{" << log_color::reset);
        print_type_simple(L, -2, prio, true);
        log_plain_noln(", ");
        if (lua_istable(L, -1) && (max_level < 0 || level < max_level)) {
            print_table(L, -1, prio, level + 1, max_level);
        } else {
            print_type_simple(L, -1, prio, true, false);
        }
        log_plain_noln(log_color::white << "}" << log_color::reset);
        lua_pop(L, 1);
    }
    log_plain_noln(log_color::yellow << "}" << log_color::reset);
}

int lua_utils::print(lua_State* L) {
    set_log_tag("lua");
    set_log_priority(log_priority::info);
    int n = lua_gettop(L);
    if (n > 0) {
        int i = 1;
        if (lua_isnumber(L, i)) {
            int prio = lua_tointeger(L, i);
            if (log::is_log_priority(prio)) {
                update_log_priority(log::to_priority(prio));
                i++;
            }
        }
        log_default_noln("");
        for (; i <= n; i++) {
            print_type(L, i, get_log_priority(), true, true);
            if (i < n) {
                log_plain_noln("\t");
            }
        }
    } else {
        log_default_noln("");
    }
    log_plain("");
    return 0;
}

void lua_utils::dump_stack(lua_State* L) {
    set_log_priority(log_priority::debug);
    log_debug("STACK START " << L);
    int s = lua_gettop(L);
    bool nil_at_top = LUA_TNIL == lua_type(L, s);
    for (int i = s; i > 0; i--) {
        log_debug_noln("  " << log_color::white << i << ": " << log_color::reset << lua_typename(L, lua_type(L, i))
                            << ": ");
        print_type(L, i, get_log_priority(), false, true, 1);
        log_plain("");
        if (i == s && nil_at_top) {
            lua_remove(L, s);
        }
    }
    log_debug("STACK END " << L);
    if (nil_at_top) {
        lua_pushnil(L);
    }
}

int lua_utils::traceback(lua_State* L) {
    const char* msg = lua_tostring(L, 1);
    if (msg) {
        luaL_traceback(L, L, msg, 1);
    } else if (!lua_isnoneornil(L, 1)) {
        if (!luaL_callmeta(L, 1, "__tostring")) {
            lua_pushliteral(L, "(no error message)");
        }
    }
    return 1;
}

void lua_utils::lua_requiref(lua_State* L, const char* modname, lua_CFunction openf, int glb) {
    lua_getregistry(L);
    lua_getfield(L, -1, modname); /* _LOADED[modname] */
    if (!lua_toboolean(L, -1)) {  /* package not already loaded? */
        lua_pop(L, 1);            /* remove field */
        lua_pushcfunction(L, openf);
        lua_pushstring(L, modname);   /* argument to open function */
        lua_call(L, 1, 1);            /* call 'openf' to open module */
        lua_pushvalue(L, -1);         /* make copy of module (call result) */
        lua_setfield(L, -3, modname); /* _LOADED[modname] = module */
    }
    lua_remove(L, -2); /* remove _LOADED table */
    if (glb) {
        lua_pushvalue(L, -1);      /* copy of module */
        lua_setglobal(L, modname); /* _G[modname] = module */
    }
}

}  // petrel
