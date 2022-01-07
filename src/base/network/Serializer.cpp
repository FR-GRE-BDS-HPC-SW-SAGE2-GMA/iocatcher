/*****************************************************
			PROJECT  : IO Catcher
			LICENSE  : Apache 2.0
*****************************************************/

/****************************************************/
#include <cstring>
#include <cassert>
#include "../common/Debug.hpp"
#include "Serializer.hpp"

/****************************************************/
using namespace IOC;

/****************************************************/
/**
 * Constructor of the serializer.
 * @param buffer Address of the buffer to be used to store or load the data.
 * @param size of the buffer.
**/
SerializerBase::SerializerBase(void * buffer, size_t size, SerializerAction action)
{
	//check
	assert(buffer != NULL);
	assert(size > 0);

	//setup
	this->buffer = reinterpret_cast<char*>(buffer);
	this->size = size;
	this->cursor = 0;
	this->action = action;
}

/****************************************************/
/**
 * Destructor of the serialize, currently do nothing.
**/
SerializerBase::~SerializerBase(void)
{
	//nothing
}

/****************************************************/
/**
 * Push an integer on the buffer.
 * @param value The value to push.
**/
void SerializerBase::apply(const char * fieldName, bool & value)
{
	//check
	assert(this->action == PACK || this->action == UNPACK);
	this->checkSize(fieldName, sizeof(value));

	//apply
	if (this->action == PACK)
		memcpy(this->buffer + this->cursor, &value, sizeof(value));
	else if (this->action == UNPACK)
		memcpy(&value, this->buffer + this->cursor, sizeof(value));
	else
		IOC_FATAL("Unsupported action !");

	//move formward
	this->cursor += sizeof(value);
}

/****************************************************/
/**
 * Push an integer on the buffer.
 * @param value The value to push.
**/
void SerializerBase::apply(const char * fieldName, uint32_t & value)
{
	//check
	assert(this->action == PACK || this->action == UNPACK);
	this->checkSize(fieldName, sizeof(value));

	//apply
	if (this->action == PACK)
		memcpy(this->buffer + this->cursor, &value, sizeof(value));
	else if (this->action == UNPACK)
		memcpy(&value, this->buffer + this->cursor, sizeof(value));
	else
		IOC_FATAL("Unsupported action !");

	//move formward
	this->cursor += sizeof(value);
}

/****************************************************/
/**
 * Push an integer on the buffer.
 * @param value The value to push.
**/
void SerializerBase::apply(const char * fieldName, int32_t & value)
{
	//check
	assert(this->action == PACK || this->action == UNPACK);
	this->checkSize(fieldName, sizeof(value));

	//apply
	if (this->action == PACK)
		memcpy(this->buffer + this->cursor, &value, sizeof(value));
	else if (this->action == UNPACK)
		memcpy(&value, this->buffer + this->cursor, sizeof(value));
	else
		IOC_FATAL("Unsupported action !");

	//move formward
	this->cursor += sizeof(value);
}

/****************************************************/
/**
 * Push an integer on the buffer.
 * @param value The value to push.
**/
void SerializerBase::apply(const char * fieldName, int64_t & value)
{
	//check
	assert(this->action == PACK || this->action == UNPACK);
	this->checkSize(fieldName, sizeof(value));

	//apply
	if (this->action == PACK)
		memcpy(this->buffer + this->cursor, &value, sizeof(value));
	else if (this->action == UNPACK)
		memcpy(&value, this->buffer + this->cursor, sizeof(value));
	else
		IOC_FATAL("Unsupported action !");

	//move formward
	this->cursor += sizeof(value);
}

/****************************************************/
/**
 * Push an integer on the buffer.
 * @param value The value to push.
**/
void SerializerBase::apply(const char * fieldName, uint64_t & value)
{
	//check
	assert(this->action == PACK || this->action == UNPACK);
	this->checkSize(fieldName, sizeof(value));

	//apply
	if (this->action == PACK)
		memcpy(this->buffer + this->cursor, &value, sizeof(value));
	else if (this->action == UNPACK)
		memcpy(&value, this->buffer + this->cursor, sizeof(value));
	else
		IOC_FATAL("Unsupported action !");

	//move formward
	this->cursor += sizeof(value);
}

/****************************************************/
void SerializerBase::apply(const char * fieldName, std::string & value)
{
	//check
	assert(this->action == PACK || this->action == UNPACK);

	//modes
	if (this->action == PACK) {
		this->apply(fieldName, (const std::string&)value);
	} else if (this->action == UNPACK) {
		//extract len
		uint32_t len = 0;
		this->apply(fieldName, len);

		//check as room to read
		this->checkSize(fieldName, len);

		//load
		char * str = this->buffer + this->cursor;
		assume(str[len-1] == '\0', "Got a non null terminated string !");
		value = str;

		//move forward
		this->cursor += len;
	} else {
		IOC_FATAL("Unsupported action !");
	}
}

/****************************************************/
void SerializerBase::apply(const char * fieldName, const std::string & value)
{
		//check
	assert(this->action == PACK);

	//modes
	if (this->action == PACK) {
		//calc len
		uint32_t len = value.size() + 1;

		//check size
		this->checkSize(fieldName, sizeof(uint32_t) + len);

		//push both
		this->apply(fieldName, len);
		this->apply(fieldName, value.c_str(), len);
	} else {
		IOC_FATAL("Unsupported action !");
	}
}

/****************************************************/
void SerializerBase::apply(const char * fieldName, void * buffer, size_t size)
{
	//check
	assert(buffer != NULL);
	assert(this->action == PACK || this->action == UNPACK);
	this->checkSize(fieldName, size);

	//modes
	if (this->action == PACK) {
		memcpy(this->buffer + this->cursor, buffer, size);
	} else if (this->action == UNPACK) {
		memcpy(buffer, this->buffer + this->cursor, size);
	} else {
		IOC_FATAL("Unsupported action !");
	}

	//move forward
	this->cursor += size;
}

/****************************************************/
void SerializerBase::serializeOrPoint(const char * fieldName, const char * & buffer, size_t size)
{
	//check
	assert(this->action == PACK || this->action == UNPACK);
	this->checkSize(fieldName, size);

	//modes
	if (this->action == PACK) {
		memcpy(this->buffer + this->cursor, buffer, size);
	} else if (this->action == UNPACK) {
		buffer = this->buffer + this->cursor;
	} else {
		IOC_FATAL("Unsupported action !");
	}

	//move forward
	this->cursor += size;
}

/****************************************************/
void SerializerBase::apply(const char * fieldName, const void * buffer, size_t size)
{
	//check
	assert(buffer != NULL);
	assert(this->action == PACK);
	this->checkSize(fieldName, size);

	//modes
	if (this->action == PACK) {
		memcpy(this->buffer + this->cursor, buffer, size);
	} else {
		IOC_FATAL("Unsupported action !");
	}

	//move forward
	this->cursor += size;
}

/****************************************************/
const size_t SerializerBase::getCursor(void)
{
	return this->cursor;
}

/****************************************************/
void SerializerBase::checkSize(const char * fieldName, size_t size)
{
	size_t requested = this->cursor + size;
	assumeArg(requested <= this->size, "Buffer is too small to get the new entry (%1) for field %2, size is %3, requested is %4")
		.arg(size)
		.arg(fieldName)
		.arg(this->size)
		.arg(requested)
		.end();
}
