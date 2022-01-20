/*****************************************************
*  PROJECT  : IO Catcher                             *
*  LICENSE  : Apache 2.0                             *
*  COPYRIGHT: 2020-2022 Bull SAS All rights reserved *
*****************************************************/

/****************************************************/
#include <cstring>
#include <cassert>
#include "../common/Debug.hpp"
#include "Serializer.hpp"

/****************************************************/
using namespace IOC;

/****************************************************/
#define ACTION_DEFINED(action) ((action) == SERIALIZER_PACK || (action) == SERIALIZER_UNPACK || (action) == SERIALIZER_STRINGIFY || (action) == SERIALIZER_SIZE)

/****************************************************/
/**
 * Constructor of the serializer.
 * @param buffer Address of the buffer to be used to store or load the data.
 * @param size of the buffer.
 * @param acion The action to be performed by the serializer.
**/
SerializerBase::SerializerBase(void * buffer, size_t size, SerializerAction action)
{
	//check
	if (action != SERIALIZER_UNSET && action != SERIALIZER_SIZE) {
		assert(buffer != NULL);
		assert(size > 0);
	}
	assert(action != SERIALIZER_STRINGIFY);

	//setup
	this->buffer = reinterpret_cast<char*>(buffer);
	this->size = size;
	this->cursor = 0;
	this->action = action;
	this->out = NULL;
	this->outFirst = false;
	this->root = true;
}

/****************************************************/
/**
 * Serialize the object into a string format in the output stream.
 * @param out Pointer to the output stream to be used.
**/
SerializerBase::SerializerBase(std::ostream * out)
{
	//check
	assert(out != NULL);

	//setup
	this->buffer = NULL;
	this->size = 0;
	this->cursor = 0;
	this->action = SERIALIZER_STRINGIFY;
	this->out = out;
	this->outFirst = true;
	this->root = true;
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
 * Apply the serialization or deserialization on a boolean value.
 * @param fieldName Name of the value to be used for stringification and error 
 * message in case of buffer overflow.
 * @param value The value to push.
**/
void SerializerBase::apply(const char * fieldName, bool & value)
{
	//check
	assert(ACTION_DEFINED(this->action));
	this->checkSize(fieldName, sizeof(value));

	//apply
	switch(this->action) {
		case SERIALIZER_PACK:
			memcpy(this->buffer + this->cursor, &value, sizeof(value));
			break;
		case SERIALIZER_UNPACK:
			memcpy(&value, this->buffer + this->cursor, sizeof(value));
			break;
		case SERIALIZER_STRINGIFY:
			*out << (this->outFirst ? "" : ", ") << fieldName << ": " << (value ? "true" : "false");
			break;
		case SERIALIZER_SIZE:
			//no data movement
			break;
		default:
			IOC_FATAL_ARG("Unsupported action %1 !").arg(this->action).end();
			break;
	}

	//move forward
	this->cursor += sizeof(value);
	this->outFirst = false;
}

/****************************************************/
/**
 * Apply the serialization or deserialization on an integer value.
 * @param fieldName Name of the value to be used for stringification and error 
 * message in case of buffer overflow.
 * @param value The value to push.
**/
void SerializerBase::apply(const char * fieldName, uint32_t & value)
{
	//check
	assert(ACTION_DEFINED(this->action));
	this->checkSize(fieldName, sizeof(value));

	//apply
	switch(this->action) {
		case SERIALIZER_PACK:
			memcpy(this->buffer + this->cursor, &value, sizeof(value));
			break;
		case SERIALIZER_UNPACK:
			memcpy(&value, this->buffer + this->cursor, sizeof(value));
			break;
		case SERIALIZER_STRINGIFY:
			*out << (this->outFirst ? "" : ", ") << fieldName << ": " << value;
			break;
		case SERIALIZER_SIZE:
			//no data movement
			break;
		default:
			IOC_FATAL_ARG("Unsupported action %1 !").arg(this->action).end();
			break;
	}

	//move forward
	this->cursor += sizeof(value);
	this->outFirst = false;
}

/****************************************************/
/**
 * Apply the serialization or deserialization on an integer value.
 * @param fieldName Name of the value to be used for stringification and error 
 * message in case of buffer overflow.
 * @param value The value to push.
**/
void SerializerBase::apply(const char * fieldName, int32_t & value)
{
	//check
	assert(ACTION_DEFINED(this->action));
	this->checkSize(fieldName, sizeof(value));

	//apply
	switch(this->action) {
		case SERIALIZER_PACK:
			memcpy(this->buffer + this->cursor, &value, sizeof(value));
			break;
		case SERIALIZER_UNPACK:
			memcpy(&value, this->buffer + this->cursor, sizeof(value));
			break;
		case SERIALIZER_STRINGIFY:
			*out << (this->outFirst ? "" : ", ") << fieldName << ": " << value;
			break;
		case SERIALIZER_SIZE:
			//no data movement
			break;
		default:
			IOC_FATAL_ARG("Unsupported action %1 !").arg(this->action).end();
			break;
	}

	//move forward
	this->cursor += sizeof(value);
	this->outFirst = false;
}

/****************************************************/
/**
 * Apply the serialization or deserialization on an integer value.
 * @param fieldName Name of the value to be used for stringification and error 
 * message in case of buffer overflow.
 * @param value The value to push.
**/
void SerializerBase::apply(const char * fieldName, int64_t & value)
{
	//check
	assert(ACTION_DEFINED(this->action));
	this->checkSize(fieldName, sizeof(value));

	//apply
	switch(this->action) {
		case SERIALIZER_PACK:
			memcpy(this->buffer + this->cursor, &value, sizeof(value));
			break;
		case SERIALIZER_UNPACK:
			memcpy(&value, this->buffer + this->cursor, sizeof(value));
			break;
		case SERIALIZER_STRINGIFY:
			*out << (this->outFirst ? "" : ", ") << fieldName << ": " << value;
			break;
		case SERIALIZER_SIZE:
			//no data movement
			break;
		default:
			IOC_FATAL_ARG("Unsupported action %1 !").arg(this->action).end();
			break;
	}

	//move forward
	this->cursor += sizeof(value);
	this->outFirst = false;
}

/****************************************************/
/**
 * Apply the serialization or deserialization on an integer value.
 * @param fieldName Name of the value to be used for stringification and error 
 * message in case of buffer overflow.
 * @param value The value to push.
**/
void SerializerBase::apply(const char * fieldName, uint64_t & value)
{
	//check
	assert(ACTION_DEFINED(this->action));
	this->checkSize(fieldName, sizeof(value));

	//apply
	switch(this->action) {
		case SERIALIZER_PACK:
			memcpy(this->buffer + this->cursor, &value, sizeof(value));
			break;
		case SERIALIZER_UNPACK:
			memcpy(&value, this->buffer + this->cursor, sizeof(value));
			break;
		case SERIALIZER_STRINGIFY:
			*out << (this->outFirst ? "" : ", ") << fieldName << ": " << value;
			break;
		case SERIALIZER_SIZE:
			//no data movement
			break;
		default:
			IOC_FATAL_ARG("Unsupported action %1 !").arg(this->action).end();
			break;
	}

	//move forward
	this->cursor += sizeof(value);
	this->outFirst = false;
}

/****************************************************/
/**
 * Apply the serialization or deserialization on a string value.
 * @param fieldName Name of the value to be used for stringification and error 
 * message in case of buffer overflow.
 * @param value The value to push.
**/
void SerializerBase::apply(const char * fieldName, std::string & value)
{
	//check
	assert(ACTION_DEFINED(this->action));

	//modes
	switch(this->action) {
		case SERIALIZER_PACK:
			//forward to the serialize only version
			this->apply(fieldName, (const std::string&)value);
			break;
		case SERIALIZER_UNPACK:
			{
				//extract len
				uint64_t len = 0;
				this->apply(fieldName, len);

				//check as room to read
				this->checkSize(fieldName, len);

				//load
				char * str = this->buffer + this->cursor;
				assumeArg(str[len-1] == '\0', "Got a non null terminated string for field %1 !").arg(fieldName).end();
				value = str;

				//move forward
				this->cursor += len;
			}
			break;
		case SERIALIZER_STRINGIFY:
			*out << (this->outFirst ? "" : ", ") << fieldName << ": \"" << value << "\"";
			break;
		case SERIALIZER_SIZE:
			//no data movement
			break;
		default:
			IOC_FATAL_ARG("Unsupported action %1 !").arg(this->action).end();
			break;
	}

	//move
	this->outFirst = false;
}

/****************************************************/
/**
 * Apply the serialization on a string value.
 * @param fieldName Name of the value to be used for stringification and error 
 * message in case of buffer overflow.
 * @param value The value to push.
**/
void SerializerBase::apply(const char * fieldName, const std::string & value)
{
	//check
	assert(ACTION_DEFINED(this->action));
	assert(this->action != SERIALIZER_UNPACK);

	//modes
	switch(this->action) {
		case SERIALIZER_PACK:
			{
				//calc len
				uint64_t len = value.size() + 1;

				//check size
				this->checkSize(fieldName, sizeof(uint32_t) + len);

				//push both
				this->apply(fieldName, len);
				this->apply(fieldName, value.c_str(), len);
			}
			break;
		case SERIALIZER_UNPACK:
			IOC_FATAL("Unsupported unpack action !");
			break;
		case SERIALIZER_STRINGIFY:
			*out << (this->outFirst ? "" : ", ") << fieldName << ": \"" << value << "\"";
			break;
		case SERIALIZER_SIZE:
			//no data movement
			break;
		default:
			IOC_FATAL_ARG("Unsupported action %1 !").arg(this->action).end();
			break;
	}

	//move
	this->outFirst = false;
}

/****************************************************/
/**
 * Apply the serialization or deserialization on a raw value.
 *
 * \b Remark: This function is a low level function and let the responsibility
 * to the caller to correctly track the size in a previous member.
 *
 * Example:
 * \code
 * size_t size = 100;
 * serializer.apply("bufferSize", size);
 * char * buffer = malloc(size);
 * serializer.apply("buffer", buffer);
 * \code
 * @param fieldName Name of the value to be used for stringification and error 
 * message in case of buffer overflow.
 * @param buffer The data to push/pop
 * @param size The size of the buffer to push/pop.
**/
void SerializerBase::apply(const char * fieldName, void * buffer, size_t size)
{
	//check
	assert(buffer != NULL);
	assert(ACTION_DEFINED(this->action));
	this->checkSize(fieldName, size);

	//modes
	switch(this->action) {
		case SERIALIZER_PACK:
			memcpy(this->buffer + this->cursor, buffer, size);
			break;
		case SERIALIZER_UNPACK:
			memcpy(buffer, this->buffer + this->cursor, size);
			break;
		case SERIALIZER_STRINGIFY:
			*out << (this->outFirst ? "" : ", ") << fieldName << ": " << buffer;
			break;
		case SERIALIZER_SIZE:
			//no data movement
			break;
		default:
			IOC_FATAL_ARG("Unsupported action %1 !").arg(this->action).end();
			break;
	}

	//move forward
	this->cursor += size;
	this->outFirst = false;
}

/****************************************************/
/**
 * Apply the serialization or deserialization on a raw value. On deserialization
 * if just points the location in the original buffer do avoid a copy and temporary
 * memory allocation.
 * 
 * \b Remark: This function is a low level function and let the responsibility
 * to the caller to correctly track the size in a previous member.
 *
 * Example:
 * \code
 * size_t size = 21;
 * serializer.apply("bufferSize", size);
 * const char * buffer = "msg to push if SERIALIZER_PACK";
 * serializer.serializeOrPoint("buffer", buffer);
 * \code
 * @param fieldName Name of the value to be used for stringification and error 
 * message in case of buffer overflow.
 * @param buffer The data to push/pop
 * @param size The size of the buffer to push/pop.
**/
void SerializerBase::serializeOrPoint(const char * fieldName, const char * & buffer, size_t size)
{
	//check
	assert(ACTION_DEFINED(this->action));
	this->checkSize(fieldName, size);

	//modes
	switch(this->action) {
		case SERIALIZER_PACK:
			memcpy(this->buffer + this->cursor, buffer, size);
			break;
		case SERIALIZER_UNPACK:
			buffer = this->buffer + this->cursor;
			break;
		case SERIALIZER_STRINGIFY:
			*out << (this->outFirst ? "" : ", ") << fieldName << ": " << (void*)buffer;
			break;
		case SERIALIZER_SIZE:
			//no data movement
			break;
		default:
			IOC_FATAL_ARG("Unsupported action %1 !").arg(this->action).end();
			break;
	}

	//move forward
	this->cursor += size;
}

/****************************************************/
/**
 * Apply the serialization on a raw value.
 * @param fieldName Name of the value to be used for stringification and error 
 * message in case of buffer overflow.
 * @param buffer The data to push/pop
 * @param size The size of the buffer to push/pop.
**/
void SerializerBase::apply(const char * fieldName, const void * buffer, size_t size)
{
	//check
	assert(buffer != NULL);
	assert(ACTION_DEFINED(this->action));
	assert(this->action != SERIALIZER_UNPACK);
	this->checkSize(fieldName, size);

	//modes
	switch(this->action) {
		case SERIALIZER_PACK:
			memcpy(this->buffer + this->cursor, buffer, size);
			break;
		case SERIALIZER_UNPACK:
			IOC_FATAL("Unsupported unpack action !");
			break;
		case SERIALIZER_STRINGIFY:
			*out << (this->outFirst ? "" : ", ") << fieldName << ": \"" << buffer << "\"";
			break;
		case SERIALIZER_SIZE:
			//no data movement
			break;
		default:
			IOC_FATAL_ARG("Unsupported action %1 !").arg(this->action).end();
			break;
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
	if (this->action != SERIALIZER_STRINGIFY) {
		size_t requested = this->cursor + size;
		assumeArg(requested <= this->size, "Buffer is too small to get the new entry (%1) for field %2, size is %3, requested is %4")
			.arg(size)
			.arg(fieldName)
			.arg(this->size)
			.arg(requested)
			.end();
	}
}

/****************************************************/
/**
 * Return the configured action.
**/
SerializerAction SerializerBase::getAction(void) const
{
	return this->action;
}
