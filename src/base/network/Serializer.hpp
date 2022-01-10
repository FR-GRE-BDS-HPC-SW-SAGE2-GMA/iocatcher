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
		template <class T> static std::string stringify(T & value);
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
/**
 * Specialization for serialization.
**/
class Serializer : public SerializerBase
{
	public:
		Serializer(void * buffer, size_t size) : SerializerBase(buffer, size, PACK) {};
};

/****************************************************/
/**
 * DeSerializer for serialization.
**/
class DeSerializer : public SerializerBase
{
	public:
		DeSerializer(void) : SerializerBase(NULL, 0, UNSET) {};
		DeSerializer(const void * buffer, size_t size) : SerializerBase((void*)buffer, size, UNPACK) {};
		template <class T> void pop(T & value);
};

/****************************************************/
/**
 * Apply the serialization / deserialization on any unknown type.
 * The given type must have a applySerializerDef(SerializationBase & serializer)
 * member function to be used to know how to serialize.
 * 
 * Example:
 * @code
 * struct {
 * 	void applySerializerDef(SerializationBase & serializer) {
 * 		serialize.apply("a", this->a);
 * 		serialize.apply("b", this->b);
 * 	};
 * 	uint64_t a;
 * 	uint64_t b;
 * }
 * @endcode
**/
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
/**
 * Pop a value from the derserialize. This is just a wrapper on the apply
 * method.
 * @param value The value to be extracted from the buffer.
**/
template <class T>
void DeSerializer::pop(T & value)
{
	this->apply("pop", value);
}

/****************************************************/
/**
 * Make possible to use the stream operator to deserialize.
**/
template <class T>
T & operator << (T & out, DeSerializer & deserializer)
{
	deserializer.apply("operator <<", out);
	return out;
}

/****************************************************/
/**
 * Take structure and stringify it to ease debug messages creation.
 * @param value The value to stringify.
**/
template <class T>
std::string SerializerBase::stringify(T & value)
{
	std::stringstream sout;
	SerializerBase serialize(&sout);
	serialize.apply("value", value);
	return sout.str();
}

}

#endif //IOC_SERIALIZER_HPP
