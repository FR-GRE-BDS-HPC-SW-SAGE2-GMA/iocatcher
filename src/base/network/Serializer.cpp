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
	assert(action != STRINGIFY);

	//setup
	this->buffer = reinterpret_cast<char*>(buffer);
	this->size = size;
	this->cursor = 0;
	this->action = action;
	this->out = NULL;
	this->outFirst = false;
}

/****************************************************/
/**
 * Serialize the object into a string format in the output stream.
**/
SerializerBase::SerializerBase(std::ostream * out)
{
	//check
	assert(out != NULL);

	//setup
	this->buffer = NULL;
	this->size = 0;
	this->cursor = 0;
	this->action = STRINGIFY;
	this->out = out;
	this->outFirst = true;
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
 * Apply the serializeation or deserialization on a boolean value.
 * @param fieldName Name of the value to be used for stringification and error 
 * message in case of buffer overflow.
 * @param value The value to push.
**/
void SerializerBase::apply(const char * fieldName, bool & value)
{
	//check
	assert(this->action == PACK || this->action == UNPACK || this->action == STRINGIFY);
	this->checkSize(fieldName, sizeof(value));

	//apply
	if (this->action == PACK)
		memcpy(this->buffer + this->cursor, &value, sizeof(value));
	else if (this->action == UNPACK)
		memcpy(&value, this->buffer + this->cursor, sizeof(value));
	else if (this->action == STRINGIFY)
		*out << (this->outFirst ? "" : ", ") << fieldName << ": " << (value ? "true" : "false");
	else
		IOC_FATAL("Unsupported action !");

	//move formward
	this->cursor += sizeof(value);
	this->outFirst = false;
}

/****************************************************/
/**
 * Apply the serializeation or deserialization on an integer value.
 * @param fieldName Name of the value to be used for stringification and error 
 * message in case of buffer overflow.
 * @param value The value to push.
**/
void SerializerBase::apply(const char * fieldName, uint32_t & value)
{
	//check
	assert(this->action == PACK || this->action == UNPACK || this->action == STRINGIFY);
	this->checkSize(fieldName, sizeof(value));

	//apply
	if (this->action == PACK)
		memcpy(this->buffer + this->cursor, &value, sizeof(value));
	else if (this->action == UNPACK)
		memcpy(&value, this->buffer + this->cursor, sizeof(value));
	else if (this->action == STRINGIFY)
		*out << (this->outFirst ? "" : ", ") << fieldName << ": " << value;
	else
		IOC_FATAL("Unsupported action !");

	//move formward
	this->cursor += sizeof(value);
	this->outFirst = false;
}

/****************************************************/
/**
 * Apply the serializeation or deserialization on an integer value.
 * @param fieldName Name of the value to be used for stringification and error 
 * message in case of buffer overflow.
 * @param value The value to push.
**/
void SerializerBase::apply(const char * fieldName, int32_t & value)
{
	//check
	assert(this->action == PACK || this->action == UNPACK || this->action == STRINGIFY);
	this->checkSize(fieldName, sizeof(value));

	//apply
	if (this->action == PACK)
		memcpy(this->buffer + this->cursor, &value, sizeof(value));
	else if (this->action == UNPACK)
		memcpy(&value, this->buffer + this->cursor, sizeof(value));
	else if (this->action == STRINGIFY)
		*out << (this->outFirst ? "" : ", ") << fieldName << ": " << value;
	else
		IOC_FATAL("Unsupported action !");

	//move formward
	this->cursor += sizeof(value);
	this->outFirst = false;
}

/****************************************************/
/**
 * Apply the serializeation or deserialization on an integer value.
 * @param fieldName Name of the value to be used for stringification and error 
 * message in case of buffer overflow.
 * @param value The value to push.
**/
void SerializerBase::apply(const char * fieldName, int64_t & value)
{
	//check
	assert(this->action == PACK || this->action == UNPACK || this->action == STRINGIFY);
	this->checkSize(fieldName, sizeof(value));

	//apply
	if (this->action == PACK)
		memcpy(this->buffer + this->cursor, &value, sizeof(value));
	else if (this->action == UNPACK)
		memcpy(&value, this->buffer + this->cursor, sizeof(value));
	else if (this->action == STRINGIFY)
		*out << (this->outFirst ? "" : ", ") << fieldName << ": " << value;
	else
		IOC_FATAL("Unsupported action !");

	//move formward
	this->cursor += sizeof(value);
	this->outFirst = false;
}

/****************************************************/
/**
 * Apply the serializeation or deserialization on an integer value.
 * @param fieldName Name of the value to be used for stringification and error 
 * message in case of buffer overflow.
 * @param value The value to push.
**/
void SerializerBase::apply(const char * fieldName, uint64_t & value)
{
	//check
	assert(this->action == PACK || this->action == UNPACK || this->action == STRINGIFY);
	this->checkSize(fieldName, sizeof(value));

	//apply
	if (this->action == PACK)
		memcpy(this->buffer + this->cursor, &value, sizeof(value));
	else if (this->action == UNPACK)
		memcpy(&value, this->buffer + this->cursor, sizeof(value));
	else if (this->action == STRINGIFY)
		*out << (this->outFirst ? "" : ", ") << fieldName << ": " << value;
	else
		IOC_FATAL("Unsupported action !");

	//move formward
	this->cursor += sizeof(value);
	this->outFirst = false;
}

/****************************************************/
/**
 * Apply the serializeation or deserialization on a string value.
 * @param fieldName Name of the value to be used for stringification and error 
 * message in case of buffer overflow.
 * @param value The value to push.
**/
void SerializerBase::apply(const char * fieldName, std::string & value)
{
	//check
	assert(this->action == PACK || this->action == UNPACK || this->action == STRINGIFY);

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
	} else if (this->action == STRINGIFY) {
		*out << (this->outFirst ? "" : ", ") << fieldName << ": \"" << value << "\"";
	} else {
		IOC_FATAL("Unsupported action !");
	}

	//move
	this->outFirst = false;
}

/****************************************************/
/**
 * Apply the serializeation on a string value.
 * @param fieldName Name of the value to be used for stringification and error 
 * message in case of buffer overflow.
 * @param value The value to push.
**/
void SerializerBase::apply(const char * fieldName, const std::string & value)
{
		//check
	assert(this->action == PACK || this->action == STRINGIFY);

	//modes
	if (this->action == PACK) {
		//calc len
		uint32_t len = value.size() + 1;

		//check size
		this->checkSize(fieldName, sizeof(uint32_t) + len);

		//push both
		this->apply(fieldName, len);
		this->apply(fieldName, value.c_str(), len);
	} else if (this->action == STRINGIFY) {
		*out << (this->outFirst ? "" : ", ") << fieldName << ": \"" << value << "\"";
	} else {
		IOC_FATAL("Unsupported action !");
	}

	//move
	this->outFirst = false;
}

/****************************************************/
/**
 * Apply the serializeation or deserialization on a raw value.
 * @param fieldName Name of the value to be used for stringification and error 
 * message in case of buffer overflow.
 * @param buffer The data to push/pop
 * @param size The size of the buffer to push/pop.
**/
void SerializerBase::apply(const char * fieldName, void * buffer, size_t size)
{
	//check
	assert(buffer != NULL);
	assert(this->action == PACK || this->action == UNPACK || this->action == STRINGIFY);
	this->checkSize(fieldName, size);

	//modes
	if (this->action == PACK) {
		memcpy(this->buffer + this->cursor, buffer, size);
	} else if (this->action == UNPACK) {
		memcpy(buffer, this->buffer + this->cursor, size);
	} else if (this->action == STRINGIFY) {
		*out << (this->outFirst ? "" : ", ") << fieldName << ": " << buffer;
	} else {
		IOC_FATAL("Unsupported action !");
	}

	//move forward
	this->cursor += size;
	this->outFirst = false;
}

/****************************************************/
/**
 * Apply the serializeation or deserialization on a raw value. On deserialization
 * if just points the location in the original buffer do avoid a copy and temporary
 * memory allocation.
 * @param fieldName Name of the value to be used for stringification and error 
 * message in case of buffer overflow.
 * @param buffer The data to push/pop
 * @param size The size of the buffer to push/pop.
**/
void SerializerBase::serializeOrPoint(const char * fieldName, const char * & buffer, size_t size)
{
	//check
	assert(this->action == PACK || this->action == UNPACK || this->action == STRINGIFY);
	this->checkSize(fieldName, size);

	//modes
	if (this->action == PACK) {
		memcpy(this->buffer + this->cursor, buffer, size);
	} else if (this->action == UNPACK) {
		buffer = this->buffer + this->cursor;
	} else if (this->action == STRINGIFY) {
		*out << (this->outFirst ? "" : ", ") << fieldName << ": " << (void*)buffer;
	} else {
		IOC_FATAL("Unsupported action !");
	}

	//move forward
	this->cursor += size;
}

/****************************************************/
/**
 * Apply the serializeation or deserialization on a raw value.
 * @param fieldName Name of the value to be used for stringification and error 
 * message in case of buffer overflow.
 * @param buffer The data to push/pop
 * @param size The size of the buffer to push/pop.
**/
void SerializerBase::apply(const char * fieldName, const void * buffer, size_t size)
{
	//check
	assert(buffer != NULL);
	assert(this->action == PACK || this->action ==  STRINGIFY);
	this->checkSize(fieldName, size);

	//modes
	if (this->action == PACK) {
		memcpy(this->buffer + this->cursor, buffer, size);
	} else if (this->action == STRINGIFY) {
		*out << (this->outFirst ? "" : ", ") << fieldName << ": \"" << buffer << "\"";
	} else {
		IOC_FATAL("Unsupported action !");
	}

	//move forward
	this->cursor += size;
	this->outFirst = false;
}

/****************************************************/
/**
 * Return the cursor position in the buffer.
**/
const size_t SerializerBase::getCursor(void)
{
	return this->cursor;
}

/****************************************************/
/**
 * Internal function to perform size pre-checking to forbid any buffer overflow.
 * @param fieldName The name of the field name to be used in the error message.
 * @param size Size of the desired move.
**/
void SerializerBase::checkSize(const char * fieldName, size_t size)
{
	if (this->action != STRINGIFY) {
		size_t requested = this->cursor + size;
		assumeArg(requested <= this->size, "Buffer is too small to get the new entry (%1) for field %2, size is %3, requested is %4")
			.arg(size)
			.arg(fieldName)
			.arg(this->size)
			.arg(requested)
			.end();
	}
}
