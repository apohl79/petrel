/**
 * \file    DataPacket.h
 * \brief   Declaration of classes DataPacket
 *
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

#ifndef DATA_PACKET_H
#define DATA_PACKET_H

#include <string>
#include <vector>
#include <stdint.h>

namespace adtech {

class DataAVROPacket;
class DataPacket {
  public:
    /** @brief Default Constructor
    */
    DataPacket() {}

    /** @brief Copy Constructor
     */
    DataPacket(const DataPacket&) {}

    DataPacket& operator=(const DataPacket&) { return *this; };

    /** @brief Destructor
     */
    virtual ~DataPacket() {}

    virtual bool writeFieldInt32(int32_t value, const std::string& strFieldName) = 0;

    virtual bool writeFieldInt64(int64_t value, const std::string& strFieldName) = 0;

    /** @brief Writes a null terminated string value

        Appends the fields required for a string value with length \v count
    */

    virtual bool writeFieldNullTerminatedString(const char* pValue, const std::string& strFieldName) = 0;

    /** @brief Writes a byte array

        Appends the fields required for a string value with length \v count
    */
    virtual bool writeFieldBytes(const char* pValue, uint64_t length, const std::string& strFieldName) = 0;

    /** @brief Writes a 32-bit float

        Appends the fields required for a 32-bit float value in IEEE754
        format scalar to the internal buffer.

    */
    virtual bool writeFieldFloat(float f, const std::string& strFieldName) = 0;

    /** @brief Writes a 64-bit float

        Appends a 64-bit float value in IEEE754
        format scalar to the internal buffer.
    */
    virtual bool writeFieldDouble(double f, const std::string& strFieldName) = 0;

    /** \brief Writes null entry
    *
    *  \param strFieldName name of the field as defined in the schema
    */
    virtual bool writeFieldNull(const std::string& strFieldName) = 0;

    virtual bool doBinarize(bool bDoSchemaValidation, size_t ExtraHeaderBytesToReserve = 0) = 0;

    virtual size_t getTotalBufferSize() const = 0;

    virtual size_t getAvroBufferSize() const = 0;

    virtual const std::vector<char>& getBinaryBuffer() const = 0;

    virtual DataAVROPacket* getSubRecord(const std::string& strFieldName) = 0;
    virtual DataAVROPacket* getSubArray(const std::string& strFieldName) = 0;
    virtual bool writeFieldRecord(DataAVROPacket* value, const std::string& strFieldName) = 0;
    virtual bool writeFieldArray(DataAVROPacket* value, const std::string& strFieldName) = 0;
    virtual bool appendToArray(DataAVROPacket* value) = 0;
};

}

#endif
