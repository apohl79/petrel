#ifndef LIB_ADTECH_AVRO_H
#define LIB_ADTECH_AVRO_H

#include "library.h"

#include <memory>

namespace adtech {
class DataAVROPacket;
}

namespace petrel {
namespace lib {

class adtech_avro : public library {
  public:
    adtech_avro(lib_context* ctx) : library(ctx) {}
    static void init(lua_State*) {}

    /// Create a record
    /// TDOD: this should only create a copy of a preinitialized packet or we can reuse objects
    /// @schema The json schema to use
    int create_packet(lua_State* L);
    
    /// Get a  sub record from the schema
    /// @param type The a record to write to. This needs to be a type defined in the schema.
    int get_record(lua_State* L);

    /// Write a loaded record into the root
    int write_record(lua_State* L);

    /// Load an array
    int get_array(lua_State* L);

    /// Write a loaded array into the root
    int write_array(lua_State* L);

    // For the write methods please have a look at the DataAVROPacket.
    int write_int32(lua_State* L);
    int write_int64(lua_State* L);
    int write_string(lua_State* L);
    int write_float(lua_State* L);
    int write_double(lua_State* L);
    int write_bool(lua_State* L);

    /// Append a loaded record to a loaded array
    int append_array_record(lua_State* L);

    // For the append methods please have a look at the DataAVROPacket.
    int append_array_int32(lua_State* L);
    int append_array_int64(lua_State* L);
    int append_array_string(lua_State* L);

    /// TODO: Send interface: create and detach a fiber to do the sending. we need to get the
    /// response out before/while sending a record
    //int send...(lua_State* L);

  private:
    std::unique_ptr<adtech::DataAVROPacket> m_root;
    adtech::DataAVROPacket* m_rec = nullptr;
    adtech::DataAVROPacket* m_array = nullptr;
};

}  // lib
}  // petrel

#endif  // LIB_ADTECH_AVRO_H
