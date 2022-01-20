/*****************************************************
*  PROJECT  : IO Catcher                             *
*  LICENSE  : Apache 2.0                             *
*  COPYRIGHT: 2020-2022 Bull SAS All rights reserved *
*****************************************************/

#ifndef IOC_SERIALIZER_HPP
#define IOC_SERIALIZER_HPP

/****************************************************/
//std
#include <cstdint>
#include <string>
#include <sstream>
#include <cassert>

/****************************************************/
namespace IOC
{

/****************************************************/
/**
 * Configure the serialization mode (pack or unpack).
**/
enum SerializerAction
{
	/** Used to serialize the given entries. **/
	SERIALIZER_PACK,
	/** Used to deserialize the given entries. **/
	SERIALIZER_UNPACK,
	/** To convert to a string. **/
	SERIALIZER_STRINGIFY,
	/** Only compute size. **/
	SERIALIZER_SIZE,
	/** 
	 * To say it is not yet configured. This is used in the LibfabricClientRequest
	 * structure to be initialied empty and then configured.
	**/
	SERIALIZER_UNSET,
};

/****************************************************/
/**
 * This class is used to serialize the protocol messages from their struct
 * representation to the message buffer to be exchanged between clients 
 * and servers
 * @brief Serialization and de-serialization class for network messages.
**/
class SerializerBase
{
	public:
		SerializerBase(void * buffer, size_t size, SerializerAction action);
		SerializerBase(std::ostream * out);
		~SerializerBase(void);
		inline void apply(const char * fieldName, uint32_t & value);
		inline void apply(const char * fieldName, uint64_t & value);
		inline void apply(const char * fieldName, int32_t & value);
		inline void apply(const char * fieldName, int64_t & value);
		inline void apply(const char * fieldName, std::string & value);
		inline void apply(const char * fieldName, const std::string & value);
		inline void apply(const char * fieldName, void * buffer, size_t size);
		inline void apply(const char * fieldName, const void * buffer, size_t size);
		inline void serializeOrPoint(const char * fieldName, const char * & buffer, size_t size);
		inline void apply(const char * fieldName, bool & value);
		template <class T > void apply(const char * fieldName, T & value);
		inline const size_t getCursor(void);
		inline void * getData(void);
		inline size_t getDataSize(void);
		template <class T> static std::string stringify(T & value);
		template <class T> static size_t computeSize(T & value);
		inline SerializerAction getAction(void) const;
	private:
		inline void checkSize(const char * fieldName, size_t size);
	private:
		/** The buffer in which to serialize or to read from. **/
		char * buffer;
		/** Size of the buffer. **/
		size_t size;
		/** The cursor to move in the buffer. **/
		size_t cursor;
		/** Define the action to apply. **/
		SerializerAction action;
		/** For the stringification we allow to provide an exteral std::ostream. **/
		std::ostream * out;
		/** 
		 * Remember if it is the first member of a sub-struct for the
		 * stringification to know if we need to prepend a comma (", ")
		 * to separate the fields.
		**/
		bool outFirst;
		/**
		 * Be true when we start to stringify not to print the field name.
		 * False when we enter in the first member so we print the field name.
		**/
		bool root;
};

/****************************************************/
/**
 * Specialization for serialization.
**/
class Serializer : public SerializerBase
{
	public:
		Serializer(void * buffer, size_t size) : SerializerBase(buffer, size, SERIALIZER_PACK) {};
};

/****************************************************/
/**
 * DeSerializer for serialization.
**/
class DeSerializer : public SerializerBase
{
	public:
		DeSerializer(void) : SerializerBase(NULL, 0, SERIALIZER_UNSET) {};
		DeSerializer(const void * buffer, size_t size) : SerializerBase((void*)buffer, size, SERIALIZER_UNPACK) {};
		template <class T> void pop(T & value);
};

}

/****************************************************/
//import the all inlined implementatoin
#include "Serializer_inlined.hpp"

#endif //IOC_SERIALIZER_HPP
