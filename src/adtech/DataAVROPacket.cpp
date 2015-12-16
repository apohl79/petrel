/**
 * \file    DataAVROPacket.cpp
 * \brief   Implementation of class DataAVROPacket
 * \date    2011-07-26
 *
 * Copyright (c) 2011 Adtech
 *
 * All rights reserved.  This software contains valuable confidential
 * and proprietary information of Adtech and is subject to applicable
 * licensing agreements.  Unauthorized reproduction, transmission, or
 * distribution of this file and its contents is a violation of
 * applicable laws.
 *
 */

#include "DataAVROPacket.h"
#include <cassert>
#include <fstream>
#include <stdexcept>
#include <sstream>
#include <avro.h>

namespace adtech {

DataAVROPacket::DataAVROPacket()
    : DataPacket(),
      m_pMyAvroSchema(NULL),
      m_pMyAvroData(NULL),
      m_vBinaryBuffer(0, 0),
      m_ExtraHeaderBufferBytes(0),
      m_initFailed(true),
      m_wasBinarized(false),
      m_isChildNode(false) {}

DataAVROPacket::DataAVROPacket(avro_schema_t* pAvroSchema)
    : DataPacket(),
      m_pMyAvroSchema(pAvroSchema),
      m_pMyAvroData(NULL),
      m_vBinaryBuffer(0, 0),
      m_ExtraHeaderBufferBytes(0),
      m_initFailed(false),
      m_wasBinarized(false),
      m_isChildNode(true) {
    m_pMyAvroData = new avro_datum_t;
    *m_pMyAvroData = avro_datum_from_schema(*m_pMyAvroSchema);

    if (*m_pMyAvroData == NULL) {
        std::stringstream ss;
        ss << "Error generating container from schema: " << avro_strerror();
        avro_schema_decref(*m_pMyAvroSchema);
        delete m_pMyAvroSchema;
        delete m_pMyAvroData;
        throw std::runtime_error(ss.str());
    }
}

DataAVROPacket::DataAVROPacket(const DataAVROPacket& other)
    : DataPacket(),
      m_pMyAvroSchema(NULL),
      m_pMyAvroData(NULL),
      m_ExtraHeaderBufferBytes(other.m_ExtraHeaderBufferBytes),
      m_initFailed(false),
      m_wasBinarized(false),
      m_isChildNode(false) {
    m_pMyAvroSchema = new avro_schema_t(NULL);
    *m_pMyAvroSchema = avro_schema_incref(*(other.m_pMyAvroSchema));

    if (*m_pMyAvroSchema == NULL) {
        std::stringstream ss;
        ss << "Copy failed: " << avro_strerror();
        delete m_pMyAvroSchema;
        throw std::runtime_error(ss.str());
    }

    m_pMyAvroData = new avro_datum_t;
    *m_pMyAvroData = avro_datum_from_schema(*m_pMyAvroSchema);

    if (*m_pMyAvroData == NULL) {
        std::stringstream ss;
        ss << "[DataAVROPacket::DataAVROPacket] Error generating container from schema: " << avro_strerror();
        avro_schema_decref(*m_pMyAvroSchema);
        delete m_pMyAvroSchema;
        delete m_pMyAvroData;
        throw std::runtime_error(ss.str());
    }
}

DataAVROPacket::~DataAVROPacket() {
    if (m_pMyAvroSchema || m_pMyAvroData) {
        destroyAVRO();
    }
}
void DataAVROPacket::destroyAVRO() {
    if (!m_initFailed) {  // would crash the adserver if schema init fails
        avro_schema_decref(*m_pMyAvroSchema);
        avro_datum_decref(*m_pMyAvroData);
    }
    delete m_pMyAvroSchema;
    m_pMyAvroSchema = 0;
    delete m_pMyAvroData;
    m_pMyAvroData = 0;

    VecSubObjects::iterator itr = m_vecSubObjects.begin();
    VecSubObjects::iterator itrE = m_vecSubObjects.end();
    for (; itr != itrE; itr++) {
        delete (*itr);
    }

    m_vecSubObjects.clear();
}

bool DataAVROPacket::initFromFile(const std::string& fileName) {
    if (m_pMyAvroSchema || m_pMyAvroData) {
        log_err("This method is meant to be called only once and not on a copy of the original object");
        return false;
    } else if (fileName.length() == 0) {
        log_err("Passed string is empty");
        return false;
    }
    // read schema
    std::ifstream schemafile(fileName.c_str());
    if (!schemafile) {
        log_err("Error reading json schema!");
        return false;
    }
    std::string str((std::istreambuf_iterator<char>(schemafile)), std::istreambuf_iterator<char>());
    schemafile.close();
    return initFromString(str);
}

bool DataAVROPacket::initFromString(const std::string& strJsonSchema) {
    if (m_pMyAvroSchema || m_pMyAvroData) {
        log_err("This method is meant to be called only once and not on a copy of the original object");
        return false;
    } else if (strJsonSchema.length() == 0) {
        log_err("Passed string is empty");
        return false;
    }

    avro_schema_error_t error;
    m_pMyAvroSchema = new avro_schema_t(NULL);

    if (avro_schema_from_json(strJsonSchema.c_str(), strJsonSchema.length(), m_pMyAvroSchema, &error)) {
        log_err("Error parsing schema: " << avro_strerror());
        delete m_pMyAvroSchema;
        m_pMyAvroSchema = NULL;
        return false;
    }
    m_pMyAvroData = new avro_datum_t;
    *m_pMyAvroData = avro_record(*m_pMyAvroSchema);

    if (*m_pMyAvroData == NULL) {
        log_err("Error generating container from schema: " << avro_strerror());
        avro_schema_decref(*m_pMyAvroSchema);
        delete m_pMyAvroSchema;
        delete m_pMyAvroData;
        m_pMyAvroData = NULL;
        m_pMyAvroSchema = NULL;
        return false;
    }
    m_initFailed = false;
    return true;
}

bool DataAVROPacket::writeDatum(const std::string& strFieldName, const avro_datum_t& field, int type,
                                const char* sourceMethod) {
    if (m_initFailed) {
        log_err("m_initFailed is set");
        return false;
    }
    if (m_wasBinarized) {
        log_err("m_wasBinarized is set");
        return false;
    }
    avro_schema_t field_schema = avro_schema_record_field_get(*m_pMyAvroSchema, strFieldName.c_str());
    // make sure to not crash even if somebody tried to write the wrong field ( works only with -O0 )
    if (field_schema == NULL) {
        log_err("NULL ptr errors, type: " << type << ", \"" << strFieldName << "\" called by " << sourceMethod
                                          << ", msg: " << avro_strerror());
        return false;
    }
    if (avro_typeof(field_schema) == AVRO_UNION) {
        int icntr = 0;
        avro_schema_t branch;
        bool found = false;

        for (icntr = 0; icntr < MAX_ITERATIONS_PER_UNION; icntr++) {
            if ((branch = avro_schema_union_branch(field_schema, icntr)) == NULL) {
                log_err("union without field type " << type << " in field \"" << strFieldName << "\", called by "
                                                    << sourceMethod << " msg: " << avro_strerror());
                return false;
            }
            if (avro_typeof(branch) == type) {
                found = true;
                break;
            }
        }
        if (!found) {
            log_err("Type: " << type << ",\"" << strFieldName << "\" called by " << sourceMethod << " not found");
            return false;
        }
        avro_datum_t union_field = avro_union(field_schema, icntr, field);
        if (avro_record_set(*m_pMyAvroData, strFieldName.c_str(), union_field)) {
            log_err("Error writing field type: " << type << ", in union field \"" << strFieldName << "\", called by "
                                                 << sourceMethod << ", msg: " << avro_strerror());
            avro_datum_decref(union_field);
            return false;
        }
        avro_datum_decref(union_field);
    } else if (avro_typeof(field_schema) == type) {
        if (avro_record_set(*m_pMyAvroData, strFieldName.c_str(), field)) {
            log_err("Error writing field type: " << type << ", in field \"" << strFieldName << "\", called by "
                                                 << sourceMethod << ", msg:" << avro_strerror());
            return false;
        }
    }
    return true;
}

bool DataAVROPacket::writeFieldInt32(int32_t value, const std::string& strFieldName) {
    avro_datum_t field = avro_int32(value);
    const bool berr = writeDatum(strFieldName, field, AVRO_INT32, __FUNCTION__);
    avro_datum_decref(field);
    return berr;
}

bool DataAVROPacket::writeFieldInt64(int64_t value, const std::string& strFieldName) {
    avro_datum_t field = avro_int64(value);
    const bool berr = writeDatum(strFieldName, field, AVRO_INT64, __FUNCTION__);
    avro_datum_decref(field);
    return berr;
}

bool DataAVROPacket::writeFieldNull(const std::string& strFieldName) {
    avro_datum_t field = avro_null();
    const bool berr = writeDatum(strFieldName, field, AVRO_NULL, __FUNCTION__);
    avro_datum_decref(field);
    return berr;
}

bool DataAVROPacket::writeFieldNullTerminatedString(const char* pValue, const std::string& strFieldName) {
    avro_datum_t field = avro_string(pValue);
    const bool berr = writeDatum(strFieldName, field, AVRO_STRING, __FUNCTION__);
    avro_datum_decref(field);
    return berr;
}

bool DataAVROPacket::writeFieldString(const std::string& strValue, const std::string& strFieldName) {
    avro_datum_t field = avro_string(strValue.c_str());
    const bool berr = writeDatum(strFieldName, field, AVRO_STRING, __FUNCTION__);
    avro_datum_decref(field);
    return berr;
}

bool DataAVROPacket::writeFieldStringMaxLength(const std::string& strValue, const size_t length,
                                               const std::string& strFieldName) {
    return writeFieldString(strValue.substr(0, std::min(length - 1, strValue.length())), strFieldName);
}

bool DataAVROPacket::writeFieldBytes(const char* pValue, uint64_t length, const std::string& strFieldName) {
    avro_datum_t field = avro_bytes(pValue, length);
    const bool berr = writeDatum(strFieldName, field, AVRO_BYTES, __FUNCTION__);
    avro_datum_decref(field);
    return berr;
}

bool DataAVROPacket::writeFieldFloat(float value, const std::string& strFieldName) {
    avro_datum_t field = avro_float(value);
    const bool berr = writeDatum(strFieldName, field, AVRO_FLOAT, __FUNCTION__);
    avro_datum_decref(field);
    return berr;
}

bool DataAVROPacket::writeFieldDouble(double value, const std::string& strFieldName) {
    avro_datum_t field = avro_double(value);
    const bool berr = writeDatum(strFieldName, field, AVRO_DOUBLE, __FUNCTION__);
    avro_datum_decref(field);
    return berr;
}

bool DataAVROPacket::writeFieldBoolean(bool value, const std::string& strFieldName) {
    avro_datum_t field = avro_boolean(value);
    const bool berr = writeDatum(strFieldName, field, AVRO_BOOLEAN, __FUNCTION__);
    avro_datum_decref(field);
    return berr;
}

std::string DataAVROPacket::getJSONSchema() const {
    if (!m_pMyAvroSchema) {
        log_err("AVRO Schema object not available!");
        return "";
    }
    std::vector<char> vSchemaBuffer(MAX_SCHEMA_BUFFER, 0);
    avro_writer_t SchemaWriter = avro_writer_memory(&vSchemaBuffer[0], vSchemaBuffer.size());
    avro_schema_to_json(*m_pMyAvroSchema, SchemaWriter);
    const size_t actuallyWrittenData = avro_writer_tell(SchemaWriter);
    if (actuallyWrittenData >= vSchemaBuffer.size()) {
        log_err("Writer Buffer exceeded!");
    }
    avro_writer_free(SchemaWriter);
    return std::string(&(vSchemaBuffer[0]), actuallyWrittenData);
}

bool DataAVROPacket::doBinarize(bool bDoSchemaValidation, size_t ExtraHeaderBytesToReserve) {
    if (m_wasBinarized) {
        return true;
    }

    m_ExtraHeaderBufferBytes = ExtraHeaderBytesToReserve;
    avro_writer_t dummyWriter = avro_writer_memory(NULL, 0);  // dummy writer required by the size precalculation method
    const int64_t size = (avro_size_data(dummyWriter, NULL, *m_pMyAvroData) *
                          BINARY_BUFFER_MULTIPLICATOR);  // precalculate the required buffer size
    avro_writer_free(dummyWriter);

    m_vBinaryBuffer.resize(size + ExtraHeaderBytesToReserve, 0);  // adjust the buffer
    avro_writer_t myWriter = avro_writer_memory(&m_vBinaryBuffer[ExtraHeaderBytesToReserve], m_vBinaryBuffer.size());

    if (avro_write_data(myWriter, bDoSchemaValidation ? *m_pMyAvroSchema : NULL,
                        *m_pMyAvroData)) {  // actually write the binaries
        log_err("Error writing record! " << avro_strerror());
        m_vBinaryBuffer.clear();
        avro_writer_free(myWriter);
        return false;
    }

    const int actuallyWrittenData(
        avro_writer_tell(myWriter));  // we need to recalc the size because avro_size_data() seems to be buggy
    if (size != actuallyWrittenData) {
        if (size > actuallyWrittenData) {
            m_vBinaryBuffer.resize(actuallyWrittenData + ExtraHeaderBytesToReserve);
        } else {
            log_err("Actual written binary ( "
                    << actuallyWrittenData << " Bytes) does not match the precalculated value( " << size << "Bytes) !");
            m_vBinaryBuffer.clear();
            return false;
        }
    }
    avro_writer_flush(myWriter);
    avro_writer_free(myWriter);
    m_wasBinarized = true;
    destroyAVRO();
    return true;
}

const std::vector<char>& DataAVROPacket::getBinaryBuffer() const { return m_vBinaryBuffer; }

size_t DataAVROPacket::getTotalBufferSize() const { return m_vBinaryBuffer.size(); }

size_t DataAVROPacket::getAvroBufferSize() const { return getTotalBufferSize() - m_ExtraHeaderBufferBytes; }

bool DataAVROPacket::writeFieldRecord(DataAVROPacket* value, const std::string& strFieldName) {
    return writeDatum(strFieldName, *(value->m_pMyAvroData), AVRO_RECORD, __FUNCTION__);
}

bool DataAVROPacket::writeFieldArray(DataAVROPacket* value, const std::string& strFieldName) {
    return writeDatum(strFieldName, *(value->m_pMyAvroData), AVRO_ARRAY, __FUNCTION__);
}

bool DataAVROPacket::appendToArray(DataAVROPacket* value) {
    if (avro_typeof(*m_pMyAvroSchema) != AVRO_ARRAY) {
        log_err("avro_typeof(field_schema) != AVRO_ARRAY");
        return false;
    }
    if (avro_array_append_datum(*m_pMyAvroData, *(value->m_pMyAvroData))) {
        log_err("avro_array_append_datum failed, msg: " << avro_strerror());
        return false;
    }
    return true;
}

bool DataAVROPacket::appendToArray(int32_t value) {
    if (avro_typeof(*m_pMyAvroSchema) != AVRO_ARRAY) {
        log_err("avro_typeof(field_schema) != AVRO_ARRAY");
        return false;
    }
    avro_datum_t field = avro_int32(value);
    if (avro_array_append_datum(*m_pMyAvroData, field)) {
        log_err("avro_array_append_datum failed, msg: " << avro_strerror());
        return false;
    }
    avro_datum_decref(field);
    return true;
}

bool DataAVROPacket::appendToArray(int64_t value) {
    if (avro_typeof(*m_pMyAvroSchema) != AVRO_ARRAY) {
        log_err("avro_typeof(field_schema) != AVRO_ARRAY");
        return false;
    }
    avro_datum_t field = avro_int64(value);
    if (avro_array_append_datum(*m_pMyAvroData, field)) {
        log_err("avro_array_append_datum failed, msg: " << avro_strerror());
        return false;
    }
    avro_datum_decref(field);
    return true;
}

bool DataAVROPacket::appendToArray(const std::string& strValue) {
    if (avro_typeof(*m_pMyAvroSchema) != AVRO_ARRAY) {
        log_err("avro_typeof(field_schema) != AVRO_ARRAY");
        return false;
    }
    avro_datum_t field = avro_string(strValue.c_str());
    if (avro_array_append_datum(*m_pMyAvroData, field)) {
        log_err("avro_array_append_datum failed, msg: " << avro_strerror());
        return false;
    }
    avro_datum_decref(field);
    return true;
}

DataAVROPacket* DataAVROPacket::getSubRecord(const std::string& strFieldName) {
    return getSubStructure(strFieldName, AVRO_RECORD);
}

DataAVROPacket* DataAVROPacket::getSubArray(const std::string& strFieldName) {
    return getSubStructure(strFieldName, AVRO_ARRAY);
}

DataAVROPacket* DataAVROPacket::getSubStructure(const std::string& strFieldName, int avro_type) {
    avro_schema_t field_schema = NULL;
    switch (avro_typeof(*m_pMyAvroSchema)) {
        case AVRO_ARRAY:
            field_schema = avro_schema_array_items(*m_pMyAvroSchema);
            break;
        case AVRO_RECORD:
            field_schema = avro_schema_get_subschema(*m_pMyAvroSchema, strFieldName.c_str());
            break;
        default:
            log_err("unsupported type");
            return NULL;
    }
    avro_schema_t* pOutSchema = new avro_schema_t;
    if (avro_typeof(field_schema) == AVRO_UNION) {
        avro_schema_t branch;
        for (int icntr = 0; icntr < MAX_ITERATIONS_PER_UNION; icntr++) {
            if ((branch = avro_schema_union_branch(field_schema, icntr)) == NULL) {
                log_err("union without record in field \"" << strFieldName << "\"" << avro_strerror());
                avro_schema_decref(field_schema);
                avro_schema_decref(branch);
                delete pOutSchema;
                return NULL;
            } else if (avro_typeof(branch) == avro_type) {
                break;
            }
        }
        *pOutSchema = avro_schema_incref(branch);
    } else if (avro_typeof(field_schema) == avro_type) {
        *pOutSchema = avro_schema_incref(field_schema);
    } else {
        log_err("unknown type!");
        delete pOutSchema;
        return NULL;
    }

    m_vecSubObjects.push_back(new DataAVROPacket(pOutSchema));  // passing ownership of pOutSchema to out output object.
    return m_vecSubObjects.back();
}

bool DataAVROPacket::setContentFromBinary(const char* ccpBinaryEncodedContendBuffer, size_t BufferSize) {
    if (!ccpBinaryEncodedContendBuffer) {
        log_err("ccpBinaryEncodedContendBuffer == NULL");
        return false;
    }
    if (BufferSize <= 0) {
        log_err("BufferSize <= 0");
        return false;
    }
    if (!m_pMyAvroSchema) {
        log_err("No schema present");
        return false;
    }
    avro_datum_decref(*m_pMyAvroData);
    delete m_pMyAvroData;
    m_pMyAvroData = new avro_datum_t;
    avro_reader_t reader = avro_reader_memory(ccpBinaryEncodedContendBuffer, BufferSize);
    bool bret = true;
    if (avro_read_data(reader, *m_pMyAvroSchema, NULL, m_pMyAvroData)) {
        log_err(
            "actual reading failed!"
            ", msg:"
            << avro_strerror());
        bret = false;
    }
    avro_reader_free(reader);
    return bret;
}

bool operator==(const DataAVROPacket& lhs, const DataAVROPacket& rhs) {
    set_log_tag("data_avro_packet");
    if (!(lhs.m_pMyAvroSchema) && !(rhs.m_pMyAvroSchema)) {
        log_debug("No data object present");
        return true;
    }

    if (!(lhs.m_pMyAvroSchema) || !(rhs.m_pMyAvroSchema)) {
        log_debug("Only one data object  present");
        return false;
    }

    return (avro_datum_equal(*(lhs.m_pMyAvroSchema), *(rhs.m_pMyAvroData)) != 0);
}

std::ostream& operator<<(std::ostream& out, const DataAVROPacket& in) {
    char* json = NULL;
    avro_datum_to_json(*(in.m_pMyAvroData), 1, &json);
    if (json == NULL) {
        set_log_tag("data_avro_packet");
        log_err("Error: " << avro_strerror());
    }
    out << json;
    free(json);
    return out;
}

}
