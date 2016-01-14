/*
 * Copyright (c) 2016 Andreas Pohl
 * Licensed under MIT (see COPYING)
 *
 * Author: Andreas Pohl
 */

#ifndef EXAMPLE_H
#define EXAMPLE_H

#include <petrel.h>

using namespace petrel::lib;

/// example class
class example : public library {
  public:
    example(lib_context* ctx) : library(ctx) {}
    /// load gets called after the lib has been loaded by petrel.
    static void load() {}
    /// unload gets called when the library gets closed.
    static void unload() {}
    /// state_init gets called for every new lua state that gets created.
    static void state_init(lua_State*) {}

    /// Some static method.
    /// A static function can be called from lua via <libname>.<funcbame>. In this example you would call:
    ///
    ///   result = example.work_static(1, 2)
    static int work_static(lua_State* L);

    /// A member function
    /// To call a member function you need to create an instance of the library first and than you can use the ':'
    /// notation:
    ///
    ///   inst = example()
    ///   result = inst:work_member("test")
    int work_member(lua_State* L);
};

#endif  // EXAMPLE_H
