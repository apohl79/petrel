#ifndef LIBRARY_H
#define LIBRARY_H

#include "lua_engine.h"
#include "server.h"

#include <functional>
#include <memory>
#include <vector>

namespace petrel {
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
    library(lib_context* ctx) : m_context(ctx), m_iosvc(ctx->io_service()) {}
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

/// Register a class as library
#define REGISTER_LIB(NAME)                                                        \
    std::vector<luaL_Reg> NAME##_library_functions;                               \
    std::vector<luaL_Reg> NAME##_library_methods;                                 \
    /* Ctor: Create a new library object */                                       \
    int NAME##_new(lua_State* L) {                                                \
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
        luaL_getmetatable(L, #NAME);                                              \
        lua_setmetatable(L, userdata);                                            \
        /* create a table to hold methods and a this pointer */                   \
        lua_createtable(L, 0, NAME##_library_methods.size() + 1);                 \
        int newtab = lua_gettop(L);                                               \
        /* store a pointer to the new object (this) in newtab at array index 1 */ \
        lua_pushvalue(L, userdata);                                               \
        lua_rawseti(L, newtab, 1);                                                \
        luaL_getmetatable(L, #NAME);                                              \
        lua_setmetatable(L, newtab);                                              \
        /* add methods to newtab */                                               \
        for (auto& f : NAME##_library_methods) {                                  \
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
    int NAME##_init(lua_State* L) noexcept {                                      \
        int funcs = lua_gettop(L);                                                \
        luaL_newmetatable(L, #NAME);                                              \
        int metatab = lua_gettop(L);                                              \
        lua_pushvalue(L, funcs);                                                  \
        lua_setfield(L, metatab, "__index");                                      \
        lua_pushvalue(L, funcs);                                                  \
        lua_setfield(L, metatab, "__metatable");                                  \
        lua_newtable(L);                                                          \
        int mt = lua_gettop(L);                                                   \
        lua_pushcfunction(L, NAME##_new);                                         \
        lua_setfield(L, mt, "__call");                                            \
        lua_setmetatable(L, funcs);                                               \
        lua_pop(L, 1);                                                            \
        NAME::init(L);                                                            \
        return 0;                                                                 \
    }                                                                             \
    /* Open the library */                                                        \
    int NAME##_open(lua_State* L) noexcept {                                      \
        luaL_checkversion(L);                                                     \
        lua_createtable(L, 0, NAME##_library_functions.size() - 1);               \
        luaL_setfuncs(L, &NAME##_library_functions[0], 0);                        \
        return 1;                                                                 \
    }                                                                             \
    /* Helper to export a static library class function */                        \
    int NAME##_add_function(const char* name, lua_CFunction f) noexcept {         \
        luaL_Reg reg = {name, f};                                                 \
        if (!NAME##_library_functions.empty()) {                                  \
            NAME##_library_functions.pop_back();                                  \
        }                                                                         \
        NAME##_library_functions.push_back(reg);                                  \
        reg = {0, 0};                                                             \
        NAME##_library_functions.push_back(reg);                                  \
        return 0;                                                                 \
    }                                                                             \
    /* Helper to export a library class method */                                 \
    int NAME##_add_method(const char* name, lua_CFunction f) noexcept {           \
        luaL_Reg reg = {name, f};                                                 \
        NAME##_library_methods.push_back(reg);                                    \
        return 0;                                                                 \
    }                                                                             \
    int NAME##_dummy =                                                            \
        lua_engine::register_lib(#NAME, NAME##_open, NAME##_init) + NAME##_add_function("new", NAME##_new)

/// Register a class method as library function, that can be called from lua on a library object via obj:method(...)
#define REGISTER_LIB_METHOD(NAME, FUNC)                                                             \
    int NAME##_##FUNC##_method(lua_State* L) {                                                      \
        if (lua_istable(L, 1)) {                                                                    \
            lua_pushvalue(L, 1);   /* load the obj table */                                         \
            lua_rawgeti(L, -1, 1); /* get this pointer */                                           \
            if (lua_islightuserdata(L, -1)) {                                                       \
                auto* obj = reinterpret_cast<const NAME*>(lua_topointer(L, -1));                    \
                lua_pop(L, 2); /* clean up the stack */                                             \
                if (nullptr != obj) {                                                               \
                    lua_remove(L, 1); /* remove object table from stack */                          \
                    return const_cast<NAME*>(obj)->FUNC(L);                                         \
                }                                                                                   \
                luaL_error(L, "cast to %s* failed", #NAME);                                         \
            }                                                                                       \
            luaL_error(L, "unable to find _this");                                                  \
        }                                                                                           \
        luaL_error(L, "Invalid method call, did you mean obj:%s instead of obj.%s?", #FUNC, #FUNC); \
        return 0;                                                                                   \
    }                                                                                               \
    int NAME##_##FUNC##_m = NAME##_add_method(#FUNC, NAME##_##FUNC##_method)

/// Register a static method as library function, that can be called from lua via libname.function(...)
#define REGISTER_LIB_FUNCTION(NAME, FUNC) int NAME##_##FUNC##_f = NAME##_add_function(#FUNC, NAME::FUNC)

}  // lib
}  // petrel

#endif  // LIBRARY_H
