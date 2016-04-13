/*
 * Copyright (c) 2016 Andreas Pohl
 * Licensed under MIT (see COPYING)
 *
 * Author: Andreas Pohl
 */

#ifndef LIBRARY_H
#define LIBRARY_H

#include <functional>
#include <memory>
#include <vector>

#include <boost/asio.hpp>
#include <lua.hpp>

namespace petrel {

class server;

namespace lib {

class library;

struct lib_context {
    // The server
    petrel::server* p_server = nullptr;
    inline petrel::server& server() { return *p_server; }

    // The boost io_service
    boost::asio::io_service* p_io_service = nullptr;
    inline boost::asio::io_service& io_service() const { return *p_io_service; }

    // We keep a vector of library objects that we delete after a request is finished. We don't leave this to the gc.
    std::vector<library*>* p_objects = nullptr;
};

/// Library base helper class that provides boost coroutine support.
class library {
  public:
    explicit library(lib_context* ctx) : m_context(ctx), m_iosvc(ctx->io_service()) {}
    virtual ~library() {}
    inline lib_context& context() const { return *m_context; }
    inline boost::asio::io_service& io_service() const { return m_iosvc; }
    static lib_context& context(lua_State* L) {
        lua_getglobal(L, "petrel_context");
        auto* ctx = reinterpret_cast<lib_context*>(lua_touserdata(L, -1));
        lua_pop(L, 1);
        return *ctx;
    }

  protected:
    lib_context* m_context;
    boost::asio::io_service& m_iosvc;
};

/// Declare a class to be a library
#define DECLARE_LIB_BEGIN(NAME)                                                   \
    namespace { /* will be closed in REGISTER_LIB* */                             \
    using lib_type = NAME;                                                        \
    using lib_state_init_func_type = std::function<void(lua_State*)>;             \
    using lib_load_func_type = std::function<void()>;                             \
    const char lib_name[] = #NAME;                                                \
    lib_state_init_func_type lib_state_init = nullptr;                            \
    lib_load_func_type lib_load = nullptr;                                        \
    lib_load_func_type lib_unload = nullptr;                                      \
    std::vector<luaL_Reg> library_functions;                                      \
    std::vector<luaL_Reg> library_methods;                                        \
    /* Ctor: Create a new library object */                                       \
    int lib_new(lua_State* L) {                                                   \
        /* load the library context */                                            \
        lua_getglobal(L, "petrel_context");                                       \
        auto* ctx = reinterpret_cast<lib_context*>(lua_touserdata(L, -1));        \
        lua_pop(L, 1);                                                            \
        /* create the object and store a pointer for cleanup */                   \
        auto* obj = new NAME(ctx);                                                \
        ctx->p_objects->push_back(obj);                                           \
        /* push a pointer to the new object to the stack */                       \
        lua_pushlightuserdata(L, obj);                                            \
        int userdata = lua_gettop(L);                                             \
        luaL_getmetatable(L, lib_name);                                           \
        lua_setmetatable(L, userdata);                                            \
        /* create a table to hold methods and a this pointer */                   \
        lua_createtable(L, 0, library_methods.size() + 1);                        \
        int newtab = lua_gettop(L);                                               \
        /* store a pointer to the new object (this) in newtab at array index 1 */ \
        lua_pushvalue(L, userdata);                                               \
        lua_rawseti(L, newtab, 1);                                                \
        luaL_getmetatable(L, lib_name);                                           \
        lua_setmetatable(L, newtab);                                              \
        /* add methods to newtab */                                               \
        for (auto& f : library_methods) {                                         \
            if (f.name) {                                                         \
                lua_pushcfunction(L, f.func);                                     \
                lua_setfield(L, newtab, f.name);                                  \
            }                                                                     \
        }                                                                         \
        /* clean up the stack so it contains only the new object table */         \
        lua_remove(L, 1);                                                         \
        lua_remove(L, 1);                                                         \
        return 1;                                                                 \
    }                                                                             \
    /* Initialize a library (loading functions etc) */                            \
    int lib_init(lua_State* L) noexcept {                                         \
        int funcs = lua_gettop(L);                                                \
        luaL_newmetatable(L, lib_name);                                           \
        int metatab = lua_gettop(L);                                              \
        lua_pushvalue(L, funcs);                                                  \
        lua_setfield(L, metatab, "__index");                                      \
        lua_pushvalue(L, funcs);                                                  \
        lua_setfield(L, metatab, "__metatable");                                  \
        lua_newtable(L);                                                          \
        int mt = lua_gettop(L);                                                   \
        lua_pushcfunction(L, lib_new);                                            \
        lua_setfield(L, mt, "__call");                                            \
        lua_setmetatable(L, funcs);                                               \
        lua_pop(L, 1);                                                            \
        if (nullptr != lib_state_init) {                                          \
            lib_state_init(L);                                                    \
        }                                                                         \
        return 0;                                                                 \
    }                                                                             \
    /* Open the library */                                                        \
    int lib_open(lua_State* L) noexcept {                                         \
        /*luaL_checkversion(L);*/                                                 \
        if (library_functions.size() > 0) {                                       \
            lua_createtable(L, 0, library_functions.size() - 1);                  \
            int newtab = lua_gettop(L);                                           \
            /*luaL_setfuncs(L, &library_functions[0], 0);*/                       \
            for (auto& f : library_functions) {                                   \
                lua_pushcfunction(L, f.func);                                     \
                lua_setfield(L, newtab, f.name);                                  \
            }                                                                     \
        }                                                                         \
        return 1;                                                                 \
    }                                                                             \
    /* Helper to export a static library class function */                        \
    int lib_add_function(const char* name, lua_CFunction f) noexcept {            \
        if (nullptr != f) {                                                       \
            luaL_Reg reg = {name, f};                                             \
            /*if (!library_functions.empty()) {   */                              \
            /*    library_functions.pop_back();   */                              \
            /*}                                   */                              \
            library_functions.push_back(reg);                                     \
            /*reg = {0, 0};                     */                                \
            /*library_functions.push_back(reg); */                                \
        }                                                                         \
        return 0;                                                                 \
    }                                                                             \
    /* Helper to export a library class method */                                 \
    int lib_add_method(const char* name, lua_CFunction f) noexcept {              \
        if (nullptr != f) {                                                       \
            luaL_Reg reg = {name, f};                                             \
            library_methods.push_back(reg);                                       \
        }                                                                         \
        return 0;                                                                 \
    }                                                                             \
    /* dummy calls to prevent -Wunused-function to fire */                        \
    int hide_unused = lib_add_function(nullptr, nullptr) + lib_add_method(nullptr, nullptr)

/// If the library implements a state_init function, call this macro
#define ENABLE_LIB_STATE_INIT()                \
    int lib_enable_state_init() {              \
        lib_state_init = lib_type::state_init; \
        return 0;                              \
    }                                          \
    int lib_state_init_dummy = lib_enable_state_init()

/// If the library implements a load function, call this macro
#define ENABLE_LIB_LOAD()          \
    int lib_enable_load() {        \
        lib_load = lib_type::load; \
        return 0;                  \
    }                              \
    int lib_load_dummy = lib_enable_load()

/// If the library implements an unload function, call this macro
#define ENABLE_LIB_UNLOAD()            \
    int lib_enable_unload() {          \
        lib_unload = lib_type::unload; \
        return 0;                      \
    }                                  \
    int lib_unload_dummy = lib_enable_unload()

/// Register a class method as library function, that can be called from lua on a library object via obj:method(...)
#define ADD_LIB_METHOD(FUNC)                                                                        \
    int FUNC##_method(lua_State* L) {                                                               \
        if (lua_istable(L, 1)) {                                                                    \
            lua_pushvalue(L, 1);   /* load the obj table */                                         \
            lua_rawgeti(L, -1, 1); /* get this pointer */                                           \
            if (lua_islightuserdata(L, -1)) {                                                       \
                auto* obj = reinterpret_cast<const lib_type*>(lua_topointer(L, -1));                \
                lua_pop(L, 2); /* clean up the stack */                                             \
                if (nullptr != obj) {                                                               \
                    lua_remove(L, 1); /* remove object table from stack */                          \
                    return const_cast<lib_type*>(obj)->FUNC(L);                                     \
                }                                                                                   \
                luaL_error(L, "cast to %s* failed", lib_name);                                      \
            }                                                                                       \
            luaL_error(L, "unable to find _this");                                                  \
        }                                                                                           \
        luaL_error(L, "Invalid method call, did you mean obj:%s instead of obj.%s?", #FUNC, #FUNC); \
        return 0;                                                                                   \
    }                                                                                               \
    int FUNC##_m = lib_add_method(#FUNC, FUNC##_method)

/// Register a static method as library function, that can be called from lua via libname.function(...)
#define ADD_LIB_FUNCTION(FUNC) int FUNC##_f = lib_add_function(#FUNC, lib_type::FUNC)

/// Register a class as shared library
#define DECLARE_LIB_END()                                              \
    extern "C" {                                                       \
    const char* petrel_lib_name() { return lib_name; }                 \
    void petrel_lib_load() {                                           \
        if (nullptr != lib_load) {                                     \
            lib_load();                                                \
        }                                                              \
    }                                                                  \
    void petrel_lib_unload() {                                         \
        if (nullptr != lib_unload) {                                   \
            lib_unload();                                              \
        }                                                              \
    }                                                                  \
    int petrel_lib_init(lua_State* L) noexcept { return lib_init(L); } \
    int petrel_lib_open(lua_State* L) noexcept { return lib_open(L); } \
    }                                                                  \
    }  // namespace opened in DECLARE_LIB_BEGIN

}  // lib
}  // petrel

#endif  // LIBRARY_H
