/**
 * \file    DataAVROPacket.h
 * \brief   Declaration of classes DataAVROPacket
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

#ifndef DATAAVROPACKET_H
#define DATAAVROPACKET_H

#include <string>
#include <vector>
#include <ios>
#include "DataPacket.h"
#include "log.h"

struct avro_obj_t;

namespace adtech {

typedef struct avro_obj_t* avro_schema_t;
typedef struct avro_obj_t* avro_datum_t;

class DataAVROPacket : public DataPacket {
  public:
    set_log_tag("DataAVROPacket");

    /// defines the maximum buffer size for json schema when getJasonSchema() is called
    static const int MAX_SCHEMA_BUFFER = 10000;
    /// limits the iterations through unions
    static const int MAX_ITERATIONS_PER_UNION = 3;
    /// multiplicate the calculated size x 20 because avro calculation is not correct
    static const int BINARY_BUFFER_MULTIPLICATOR = 20;

    /** \brief Intialization Constructor
     * creates an empty instance of DataAVROPacket
     */
    DataAVROPacket();

    /**
     * \brief Copy Constructor
     * \param other source object for the copy constructor
     * \exception runtime_error if init failed
     */
    DataAVROPacket(const DataAVROPacket& other);

    DataAVROPacket& operator=(const DataAVROPacket&) { return *this; };

    /** \brief Destructor
    */
    virtual ~DataAVROPacket();

    /**
     * \brief init the object by reading the json file and apply the schema
     * This method is meant to be called one time when the adserver comes up. Printed messages are supposed to be
     * printed out!!!!!!!!!!!!!
     * \param absolute or relativ fileName to the json schema or name of the object
     * \return true if sucess
     */
    bool initFromFile(const std::string& fileName);

    /**
     * \brief init the object by reading the json string and apply the schema
     * This method is meant to be called one time when the adserver comes up. Printed messages are supposed to be
     * printed out!!!!!!!!!!!!!
     * \param json ecoded schema as a string
     * \return true if sucess
     */
    bool initFromString(const std::string& strJsonSchema);

    /**
     * \brief set the content of the object by passing a binary (AVRO) encoded buffer.
     * This method is meant to be called in unit tests. The schema must be set before the method is called.
     * \param Binary buffer array of the content and its size.
     * \return true if sucess
     */
    bool setContentFromBinary(const char* ccpBinaryEncodedContendBuffer, size_t BufferSize);

    /** \brief Write a 32bit integer field to the packet.
     *  the Field must be defined in the schema that was passed to the constructor
     *
     *  \param value the actual value to save
     *  \param strFieldName Name of the field as defined in the schema
     *
    */
    bool writeFieldInt32(int32_t value, const std::string& strFieldName);

    /** \brief Write a 64bit integer field to the packet.
     *  the Field must be defined in the schema that was passed to the constructor
     *
     *  \param value the actual value to save
     *  \param strFieldName name of the field as defined in the schema
    */
    bool writeFieldInt64(int64_t value, const std::string& strFieldName);

    /** \brief Writes a null terminated string value
    *  the Field must be defined in the schema that was passed to the constructor
    *
    *  \param pValue the pointer to the NULL-terminated string array
    *  \param strFieldName name of the field as defined in the schema
    */

    bool writeFieldNullTerminatedString(const char* pValue, const std::string& strFieldName);

    /** \brief Writes a null terminated string value
    *  the Field must be defined in the schema that was passed to the constructor
    *
    *  \param strValue the pointer to the NULL-terminated string array
    *  \param strFieldName name of the field as defined in the schema
    */

    bool writeFieldString(const std::string& strValue, const std::string& strFieldName);

    /** \brief Writes a string value, but max length-1 characters.
     *  the Field must be defined in the schema that was passed to the constructor
     *
     *  \param strValue the string to be written
     *  \param length max length to be written. If 0, ignore length limitation
     *  \param strFieldName name of the field as defined in the schema
     */
    bool writeFieldStringMaxLength(const std::string& strValue, const size_t length, const std::string& strFieldName);

    /** \brief Writes a raw byte array
    *
    *  \param pValue the pointer to the raw array
    *  \param size length of the raw array
    *  \param strFieldName name of the field as defined in the schema
   */
    bool writeFieldBytes(const char* pValue, uint64_t length, const std::string& strFieldName);

    /** \brief Writes a 32-bit float

    *  \param value the actual value to save
    *  \param strFieldName name of the field as defined in the schema
    */
    bool writeFieldFloat(float value, const std::string& strFieldName);

    /** \brief Writes a 64-bit float
   *
   *  \param value the actual value to save
   *  \param strFieldName name of the field as defined in the schema
   */
    bool writeFieldDouble(double value, const std::string& strFieldName);

    /** \brief Writes a boolean
    *
    *  \param value the actual value to save
    *  \param strFieldName name of the field as defined in the schema
    */
    bool writeFieldBoolean(bool value, const std::string& strFieldName);

    /** \brief Writes a record to the packet
    *  The field must be defined as a record in the schema of the new host container (packet)
    *  \param value pointer to the actual packet to write
    *  \param strFieldName name of the field as defined in the schema
    */
    bool writeFieldRecord(DataAVROPacket* value, const std::string& strFieldName);

    /** \brief Writes an array to the packet
    *  The field must be defined as an array in the schema of the new host container (packet)
    *  \param value pointer to the actual packet to write
    *  \param strFieldName name of the field as defined in the schema
    */
    bool writeFieldArray(DataAVROPacket* value, const std::string& strFieldName);

    /** \brief Append entry to array
    *  \param DataAVROPacket pointer of the array entry
    */
    bool appendToArray(DataAVROPacket* value);

    /** \brief Append entry to array
    *  \param int32_t value for arrays that just contain integers
    */
    bool appendToArray(int32_t value);

    /** \brief Append entry to array
    *  \param int64_t value for arrays that just contain integers
    */
    bool appendToArray(int64_t value);

    /** \brief Append entry to array
    *  \param std::string reference for arrays that just contain strings (warning the string will be copied internally)
    */
    bool appendToArray(const std::string& value);

    /** \brief Writes null entry
    *
    *  \param strFieldName name of the field as defined in the schema
    */
    bool writeFieldNull(const std::string& strFieldName);

    /** \brief returns the sub record strFieldName from the this structure
    *  Method is used to retrieve a child node. Deletion of the node happens internally so DO NOT DELETE IT OUTSIDE!
    *
    *    "type": "record",
    *    "name": "NGADataAvroPacket",
    *    "fields": [
    *    {
    *        "name": "SGSHeader",     <-------------  DataAVROPacket* myChild = ROOTNODE->getSubRecord("SGSHeader");
    *        "type": {                                myChild->writeFieldInt(1984,"VersionID");
    *            "type": "record",
    *            "name": "SGSHeader",
    *            "fields": [
    *                {
    *                    "name": "VersionID",
    *                    "type": "int"
    *                },
    *
    *  \param strFieldName name of the field as defined in the schema
    *  \return pointer to a child node which can be treated like the root except you are NOT allowed to delete it.
    * Returns NULL if failed.
    */
    DataAVROPacket* getSubRecord(const std::string& strFieldName);

    /** \brief returns the sub array strFieldName from the this structure
    *  Method is used to retrieve the a child node. Deletion of the node happens internally so DO NOT DELETE IT OUTSIDE!
    *
    *        "type": "record",
    *        "name": "SGSKeyValuePacket",
    *        "fields": [
    *            {
    *                "name": "PhraseId",
    *                "type": "int"
    *            },
    *            {
    *                "name": "KVArray",         <---------------- DataAVROPacket* myArray =
    * ROOTNODE->getSubArray("KVArray");
    *                "type":                                      DataAVROPacket *myKVEntry =
    * myArray->getSubRecord("KVEntry");
    *                    {                                        myKVEntry->writeFieldInt32(1984, "KeyId");
    *                        "type": "array","items": {           myKVArray->appendToArray(myKVEntry);
    *                            "type": "record",
    *                            "name": "KVEntry","fields": [
    *
    *  \param strFieldName name of the field as defined in the schema
    *  \return pointer to a child node which can be treated like the root except you are NOT allowed to delete it.
    * Returns NULL if failed.
    */
    DataAVROPacket* getSubArray(const std::string& strFieldName);

    /** \brief Does the actual binarization.
    *  Adding vaules to a container that is already binarized is not defined. The binarization result will be stored
    * internally
    *  in the m_vBinaryBuffer.
    *
    *  \param bDoSchemaValidation
    *  \param ExtraHeaderBytesToReserve the binarisation will leave some blank space in the internal buffer in front of
    * the actual data
    *  You can use this to add some fancy custom header information to the buffer.
    */
    bool doBinarize(bool bDoSchemaValidation, size_t ExtraHeaderBytesToReserve = 0);

    /** \brief Size of the internal binary buffer with reserved extra header bytes
    *
    */
    size_t getTotalBufferSize() const;

    /** \brief Size of the actual binary avro data in the buffer without reserved extra header bytes
    *
    */
    size_t getAvroBufferSize() const;

    /** \brief const reference to the internal binary buffer one may avoid copying it by creating
    *  a reference on his own stack
    *  \code const std::vector<char>& Result (MyAvroPacket.getBinaryBuffer()); \endcode
    *
    */
    const std::vector<char>& getBinaryBuffer() const;

    /** \brief create the jason encoded schema of the current packet.
    *  May be useful for debugging purposes
    *
    *
    */
    std::string getJSONSchema() const;

  private:
    /**
     * \brief Internal Constructor. This is used to create sub objects such as records and arrays. This objects are
     * dreated by getSubStructure(). Using this constructor marks the created object as a "child" which must be only
     * destructed by the parent.
         * \param The schema of the new sub object
         *
     * \exception runtime_error if init failed
     */
    DataAVROPacket(avro_schema_t* pAvroSchema);

    // I'm my best friend ;)
    friend bool writeFieldRecord(DataAVROPacket* value, const std::string& strFieldName);
    friend DataAVROPacket* getSubRecord(const std::string& strFieldName, int avro_type);
    friend bool appendToArray(DataAVROPacket* value, const std::string& strFieldName);
    friend std::ostream& operator<<(std::ostream& out, const DataAVROPacket& in);
    friend bool operator==(const DataAVROPacket& lhs, const DataAVROPacket& rhs);

    /** \brief sub method for all writeField*** calls
     *  For internal use only, to prevent code duplication
     *  \param strFieldName name of the field as defined in the schema
     *  \param field avro field member where the wrapper should store the information
     *  \param type ao type of the member
     *  \param sourceMethod filled by pre-processor with the method name which called the writeDatum
     *  \return true if success
     */
    bool writeDatum(const std::string& strFieldName, const avro_datum_t& field, const int type,
                    const char* sourceMethod);
    DataAVROPacket* getSubStructure(const std::string& strFieldName, int avro_type);
    void destroyAVRO();

    avro_schema_t* m_pMyAvroSchema;
    avro_datum_t* m_pMyAvroData;

    std::vector<char> m_vBinaryBuffer;
    size_t m_ExtraHeaderBufferBytes;
    bool m_initFailed;
    bool m_wasBinarized;
    bool m_isChildNode;

    typedef std::vector<DataAVROPacket*> VecSubObjects;
    VecSubObjects m_vecSubObjects;
};

/** \brief print the json encoded data of the current packet.
*   May be useful for debugging purposes
*/

std::ostream& operator<<(std::ostream& out, const DataAVROPacket& in);

bool operator==(const DataAVROPacket& lhs, const DataAVROPacket& rhs);

}

#endif  // DATA_AVRO_PACKET_H
