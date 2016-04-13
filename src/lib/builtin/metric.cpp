/*
 * Copyright (c) 2016 Andreas Pohl
 * Licensed under MIT (see COPYING)
 *
 * Author: Andreas Pohl
 */

#include <unordered_map>

#include "make_unique.h"
#include "metric.h"
#include "metrics/counter.h"
#include "metrics/meter.h"
#include "metrics/registry.h"
#include "server.h"

namespace petrel {
namespace lib {

DECLARE_LIB_BEGIN(metric);
ADD_LIB_METHOD(register_counter);
ADD_LIB_METHOD(register_meter);
ADD_LIB_METHOD(register_timer);
ADD_LIB_METHOD(get_counter);
ADD_LIB_METHOD(get_meter);
ADD_LIB_METHOD(get_timer);
ADD_LIB_METHOD(update);
ADD_LIB_METHOD(start_duration);
ADD_LIB_METHOD(update_duration);
ADD_LIB_METHOD(finish_duration);
ADD_LIB_METHOD(increment);
ADD_LIB_METHOD(decrement);
ADD_LIB_METHOD(total);
ADD_LIB_METHOD(reset);
DECLARE_LIB_BUILTIN_END();

int metric::register_counter(lua_State* L) {
    const char* name = luaL_checkstring(L, 1);
    auto& reg = context().server().get_metrics_registry();
    m_metric = reg.register_metric<metrics::counter>(name);
    m_type = type::counter;
    return 0;
}

int metric::register_meter(lua_State* L) {
    const char* name = luaL_checkstring(L, 1);
    auto& reg = context().server().get_metrics_registry();
    m_metric = reg.register_metric<metrics::meter>(name);
    m_type = type::meter;
    return 0;
}

int metric::register_timer(lua_State* L) {
    const char* name = luaL_checkstring(L, 1);
    auto& reg = context().server().get_metrics_registry();
    m_metric = reg.register_metric<metrics::timer>(name);
    m_type = type::timer;
    return 0;
}

int metric::get_counter(lua_State* L) {
    const char* name = luaL_checkstring(L, 1);
    auto& reg = context().server().get_metrics_registry();
    m_metric = reg.get_metric<metrics::counter>(name);
    m_type = type::counter;
    return 0;
}

int metric::get_meter(lua_State* L) {
    const char* name = luaL_checkstring(L, 1);
    auto& reg = context().server().get_metrics_registry();
    m_metric = reg.get_metric<metrics::meter>(name);
    m_type = type::meter;
    return 0;
}

int metric::get_timer(lua_State* L) {
    const char* name = luaL_checkstring(L, 1);
    auto& reg = context().server().get_metrics_registry();
    m_metric = reg.get_metric<metrics::timer>(name);
    m_type = type::timer;
    return 0;
}

int metric::update(lua_State* L) {
    double val = luaL_checknumber(L, 1);
    if (nullptr == m_metric) {
        luaL_error(L, "no metric assigned");
    }
    if (type::timer != m_type) {
        luaL_error(L, "invalid metric type, update works only for timer metrics");
    }
    std::dynamic_pointer_cast<metrics::timer>(m_metric)->update(val);
    return 0;
}

int metric::start_duration(lua_State* L) {
    if (nullptr == m_metric) {
        luaL_error(L, "no metric assigned");
    }
    if (type::timer != m_type) {
        luaL_error(L, "invalid metric type, start_duration works only for timer metrics");
    }
    auto timer = std::dynamic_pointer_cast<metrics::timer>(m_metric);
    m_duration = std::make_unique<metrics::timer::duration>(timer);
    return 0;
}

int metric::update_duration(lua_State* L) {
    if (nullptr == m_duration) {
        luaL_error(L, "no duration, you have to call start_duration");
    }
    m_duration->update();
    return 0;
}

int metric::finish_duration(lua_State* L) {
    if (nullptr == m_duration) {
        luaL_error(L, "no duration, you have to call start_duration");
    }
    m_duration->finish();
    return 0;
}

int metric::increment(lua_State* L) {
    int val = 1;
    if (lua_isnumber(L, 1)) {
        val = lua_tointeger(L, 1);
    }
    if (nullptr == m_metric) {
        luaL_error(L, "no metric assigned");
    }
    switch (m_type) {
        case type::counter:
            std::dynamic_pointer_cast<metrics::counter>(m_metric)->increment(val);
            break;
        case type::meter:
            std::dynamic_pointer_cast<metrics::meter>(m_metric)->increment(val);
            break;
        default:
            luaL_error(L, "invalid metric type, increment works only for counter and meter metrics");
    }
    return 0;
}

int metric::decrement(lua_State* L) {
    int val = 1;
    if (lua_isnumber(L, 1)) {
        val = lua_tointeger(L, 1);
    }
    if (nullptr == m_metric) {
        luaL_error(L, "no metric assigned");
    }
    if (type::counter != m_type) {
        luaL_error(L, "invalid metric type, decrement works only for counters");
    }
    std::dynamic_pointer_cast<metrics::counter>(m_metric)->decrement(val);
    return 0;
}

int metric::total(lua_State* L) {
    if (nullptr == m_metric) {
        luaL_error(L, "no metric assigned");
    }
    std::uint_fast64_t total = 0;
    switch (m_type) {
        case type::counter:
            total = std::dynamic_pointer_cast<metrics::counter>(m_metric)->get();
            break;
        case type::meter:
            total = std::dynamic_pointer_cast<metrics::meter>(m_metric)->total();
            break;
        default:
            luaL_error(L, "invalid metric type, total works only for counter and meter metrics");
    }
    lua_pushinteger(L, total);
    return 1;
}

int metric::reset(lua_State* L) {
    if (nullptr == m_metric) {
        luaL_error(L, "no metric assigned");
    }
    if (type::counter != m_type) {
        luaL_error(L, "invalid metric type, reset works only for counters");
    }
    std::dynamic_pointer_cast<metrics::counter>(m_metric)->get_and_reset();
    return 0;
}

}  // lib
}  // petrel
