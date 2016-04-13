/*
 * Copyright (c) 2016 Andreas Pohl
 * Licensed under MIT (see COPYING)
 *
 * Author: Andreas Pohl
 */

#ifndef LIBRARY_BUILTIN_H
#define LIBRARY_BUILTIN_H

#include "library.h"
#include "lua_state_manager.h"

/// Register a class as builtin library
#define DECLARE_LIB_BUILTIN_END()                                                                    \
    int lib_register() {                                                                             \
        lua_state_manager::register_lib_builtin(lib_name, lib_open, lib_init, lib_load, lib_unload); \
        lib_add_function("new", lib_new);                                                            \
        return 0;                                                                                    \
    }                                                                                                \
    int register_dummy = lib_register();                                                             \
    }  // namespace opened in DECLARE_LIB_BEGIN

#endif  // LIBRARY_BUILTIN_H
