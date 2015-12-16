#include <unordered_map>

#include "metric.h"
#include "metrics/registry.h"
#include "metrics/counter.h"
#include "metrics/meter.h"
#include "metrics/timer.h"

namespace petrel {
namespace lib {

REGISTER_LIB(metric);
REGISTER_LIB_METHOD(metric, register_counter);
REGISTER_LIB_METHOD(metric, register_meter);
REGISTER_LIB_METHOD(metric, register_timer);
REGISTER_LIB_METHOD(metric, get_counter);
REGISTER_LIB_METHOD(metric, get_meter);
REGISTER_LIB_METHOD(metric, get_timer);
REGISTER_LIB_METHOD(metric, update);
REGISTER_LIB_METHOD(metric, start_duration);
REGISTER_LIB_METHOD(metric, update_duration);
REGISTER_LIB_METHOD(metric, finish_duration);
REGISTER_LIB_METHOD(metric, increment);

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
        luaL_error(L, "invalid metric type, update works only timer metrics");        
    }
    std::dynamic_pointer_cast<metrics::timer>(m_metric)->update(val);
    return 0;
}

int metric::start_duration(lua_State* L) {
    if (nullptr == m_metric) {
        luaL_error(L, "no metric assigned");
    }
    if (type::timer != m_type) {
        luaL_error(L, "invalid metric type, start_duration works only timer metrics");        
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
    m_duration->reset();
    return 0;
}

int metric::increment(lua_State* L) {
    int val = 1;
    if (lua_isinteger(L, 1)) {
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
            luaL_error(L, "invalid metric type, increment works only counter and meter metrics");
    }
    return 0;
}

}  // lib
}  // petrel
