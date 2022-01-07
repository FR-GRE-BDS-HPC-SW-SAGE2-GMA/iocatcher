/*****************************************************
			 PROJECT  : IO Catcher
			 LICENSE  : Apache 2.0
			 COPYRIGHT: 2020 Bull SAS
*****************************************************/

#ifndef IOC_SERIALIZER_HPP
#define IOC_SERIALIZER_HPP

/****************************************************/
//std
#include <cstdint>
#include <string>
#include <sstream>

/****************************************************/
namespace IOC
{

/****************************************************/
/**
 * Configure the serializeation mode (pack or unpack).
**/
enum SerializerAction
{
	/** Used to sirialize the given entries. **/
	PACK,
	/** Used to deserializes the given entries. **/
	UNPACK,
	/** To string. **/
	STRINGIFY,
	/** To say it is not yet configured. **/
	UNSET,
};

/****************************************************/
/**
 * This class it used to serialize the protocol messages from their struct
 * representation to the message buffer to be exchanged between clients 
 * and servers
 * @brief Serialization and de-serializeation class for network messages.
**/
class SerializerBase
{
	public:
		SerializerBase(void * buffer, size_t size, SerializerAction action);
		SerializerBase(std::ostream * out);
		~SerializerBase(void);
		void apply(const char * fieldName, uint32_t & value);
		void apply(const char * fieldName, uint64_t & value);
		void apply(const char * fieldName, int32_t & value);
		void apply(const char * fieldName, int64_t & value);
		void apply(const char * fieldName, std::string & value);
		void apply(const char * fieldName, const std::string & value);
		void apply(const char * fieldName, void * buffer, size_t size);
		void apply(const char * fieldName, const void * buffer, size_t size);
		void serializeOrPoint(const char * fieldName, const char * & buffer, size_t size);
		void apply(const char * fieldName, bool & value);
		template <class T > void apply(const char * fieldName, T & value);
		const size_t getCursor(void);
		void * getData(void);
		size_t getDataSize(void);
		template <class T> static std::string toString(T & value);
	private:
		void checkSize(const char * fieldName, size_t size);
	private:
		char * buffer;
		size_t size;
		size_t cursor;
		SerializerAction action;
		std::ostream * out;
		bool outFirst;
};

/****************************************************/
class Serializer : public SerializerBase
{
	public:
		Serializer(void * buffer, size_t size) : SerializerBase(buffer, size, PACK) {};
};

/****************************************************/
class DeSerializer : public SerializerBase
{
	public:
		DeSerializer(void) : SerializerBase(NULL, 0, UNSET) {};
		DeSerializer(const void * buffer, size_t size) : SerializerBase((void*)buffer, size, UNPACK) {};
		template <class T> void pop(T & value);
};

/****************************************************/
template <class T >
void SerializerBase::apply(const char * fieldName, T & value)
{
	//open backet for objects
	if (this->action == STRINGIFY) {
		assert(this->out != NULL);
		*this->out << (this->outFirst ? "" : ", ") << "{ ";
		this->outFirst = true;
	}

	value.applySerializerDef(*this);

	//close backend for objects
	if (this->action == STRINGIFY)
		*this->out << (this->outFirst ? "" : " ") << "}";

	//move
	this->outFirst = false;
}

/****************************************************/
template <class T>
void DeSerializer::pop(T & value)
{
	this->apply("pop", value);
}

/****************************************************/
template <class T>
T & operator << (T & out, DeSerializer & deserializer)
{
	deserializer.apply("operator <<", out);
	return out;
}

/****************************************************/
template <class T>
std::string SerializerBase::toString(T & value)
{
	std::stringstream sout;
	SerializerBase serialize(&sout);
	serialize.apply("value", value);
	return sout.str();
}

}

#endif //IOC_SERIALIZER_HPP
