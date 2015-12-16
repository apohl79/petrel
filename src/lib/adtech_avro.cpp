#include "adtech_avro.h"
#include "adtech/DataAVROPacket.h"

namespace petrel {
namespace lib {

REGISTER_LIB(adtech_avro);
REGISTER_LIB_METHOD(adtech_avro, create_packet);
REGISTER_LIB_METHOD(adtech_avro, get_record);
REGISTER_LIB_METHOD(adtech_avro, write_record);
REGISTER_LIB_METHOD(adtech_avro, get_array);
REGISTER_LIB_METHOD(adtech_avro, write_array);
REGISTER_LIB_METHOD(adtech_avro, write_int32);
REGISTER_LIB_METHOD(adtech_avro, write_int64);
REGISTER_LIB_METHOD(adtech_avro, write_string);
REGISTER_LIB_METHOD(adtech_avro, write_float);
REGISTER_LIB_METHOD(adtech_avro, write_double);
REGISTER_LIB_METHOD(adtech_avro, write_bool);
REGISTER_LIB_METHOD(adtech_avro, append_array_record);
REGISTER_LIB_METHOD(adtech_avro, append_array_int32);
REGISTER_LIB_METHOD(adtech_avro, append_array_int64);
REGISTER_LIB_METHOD(adtech_avro, append_array_string);

int adtech_avro::create_packet(lua_State* L) {
    const char* schema = luaL_checkstring(L, 1);
    try {
        m_root = std::make_unique<adtech::DataAVROPacket>();
        m_root->initFromString(schema);
    } catch (std::runtime_error& e) {
        luaL_error(L, "failed to create record: %s", e.what());
    }
    return 0;
}

int adtech_avro::get_record(lua_State* L) {
    const char* type = luaL_checkstring(L, 1);
    if (nullptr == m_root) {
        luaL_error(L, "you need to call create_record first");
    }
    m_rec = m_root->getSubRecord(type);
    if (nullptr == m_rec) {
        luaL_error(L, "failed to get subrecord");
    }
    return 0;
}

int adtech_avro::write_record(lua_State* L) {
    const char* name = luaL_checkstring(L, 1);
    if (nullptr == m_root) {
        luaL_error(L, "you need to call create_record first");
    }
    if (nullptr == m_rec) {
        luaL_error(L, "no record loaded, you have to call get_record");
    }
    m_root->writeFieldRecord(m_rec, name);
    m_rec = nullptr;
    return 0;
}

int adtech_avro::get_array(lua_State* L) {
    const char* type = luaL_checkstring(L, 1);
    if (nullptr == m_root) {
        luaL_error(L, "you need to call create_record first");
    }
    m_array = m_root->getSubArray(type);
    if (nullptr == m_rec) {
        luaL_error(L, "failed to get array");
    }
    return 0;
}

int adtech_avro::write_array(lua_State* L) {
    const char* name = luaL_checkstring(L, 1);
    if (nullptr == m_root) {
        luaL_error(L, "you need to call create_record first");
    }
    if (nullptr == m_array) {
        luaL_error(L, "no array loaded, you have to call get_array");
    }
    m_root->writeFieldArray(m_array, name);
    m_array = nullptr;
    return 0;
}

int adtech_avro::write_int32(lua_State* L) {
    const char* name = luaL_checkstring(L, 1);
    std::int32_t val = luaL_checkinteger(L, 2);
    if (nullptr == m_rec) {
        luaL_error(L, "no record loaded, you have to call get_record");
    }
    m_rec->writeFieldInt32(val, name);
    return 0;
}

int adtech_avro::write_int64(lua_State* L) {
    const char* name = luaL_checkstring(L, 1);
    std::int64_t val = luaL_checkinteger(L, 2);
    if (nullptr == m_rec) {
        luaL_error(L, "no record loaded, you have to call get_record");
    }
    m_rec->writeFieldInt64(val, name);
    return 0;
}

int adtech_avro::write_string(lua_State* L) {
    const char* name = luaL_checkstring(L, 1);
    const char* val = luaL_checkstring(L, 2);
    if (nullptr == m_rec) {
        luaL_error(L, "no record loaded, you have to call get_record");
    }
    m_rec->writeFieldNullTerminatedString(val, name);
    return 0;
}

int adtech_avro::write_float(lua_State* L) {
    const char* name = luaL_checkstring(L, 1);
    float val = luaL_checknumber(L, 2);
    if (nullptr == m_rec) {
        luaL_error(L, "no record loaded, you have to call get_record");
    }
    m_rec->writeFieldFloat(val, name);
    return 0;
}

int adtech_avro::write_double(lua_State* L) {
    const char* name = luaL_checkstring(L, 1);
    double val = luaL_checknumber(L, 2);
    if (nullptr == m_rec) {
        luaL_error(L, "no record loaded, you have to call get_record");
    }
    m_rec->writeFieldDouble(val, name);
    return 0;
}

int adtech_avro::write_bool(lua_State* L) {
    const char* name = luaL_checkstring(L, 1);
    if (!lua_isboolean(L, 2)) {
        luaL_error(L, "bool expected as second arg");
    }
    bool val = lua_toboolean(L, 2);
    if (nullptr == m_rec) {
        luaL_error(L, "no record loaded, you have to call get_record");
    }
    m_rec->writeFieldBoolean(val, name);
    return 0;
}

int adtech_avro::append_array_record(lua_State* L) {
    if (nullptr == m_rec) {
        luaL_error(L, "no record loaded, you have to call get_record");
    }
    if (nullptr == m_array) {
        luaL_error(L, "no array loaded, you have to call get_array");
    }
    m_array->appendToArray(m_rec);
    return 0;
}

int adtech_avro::append_array_int32(lua_State* L) {
    std::int32_t val = luaL_checkinteger(L, 1);
    if (nullptr == m_array) {
        luaL_error(L, "no array loaded, you have to call get_array");
    }
    m_array->appendToArray(val);
    return 0;
}

int adtech_avro::append_array_int64(lua_State* L) {
    std::int64_t val = luaL_checkinteger(L, 1);
    if (nullptr == m_array) {
        luaL_error(L, "no array loaded, you have to call get_array");
    }
    m_array->appendToArray(val);
    return 0;
}

int adtech_avro::append_array_string(lua_State* L) {
    const char* val = luaL_checkstring(L, 1);
    if (nullptr == m_array) {
        luaL_error(L, "no array loaded, you have to call get_array");
    }
    m_array->appendToArray(val);
    return 0;
}

}  // lib
}  // petrel
