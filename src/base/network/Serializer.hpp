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
};

/****************************************************/
/**
 * Specialization for serialization.
**/
class Serializer : public SerializerBase
{
	public:
		Serializer(void * buffer, size_t size) : SerializerBase(buffer, size, SERIALIZER_PACK) {};
		template <class T> static size_t serialize(void * buffer, size_t bufferSize, T & value);
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
 * 		serializer.apply("a", this->a);
 * 		serializer.apply("b", this->b);
 * 	};
 * 	uint64_t a;
 * 	uint64_t b;
 * }
 * @endcode
**/
template <class T >
void SerializerBase::apply(const char * fieldName, T & value)
{
	//open bracket for objects
	if (this->action == SERIALIZER_STRINGIFY) {
		assert(this->out != NULL);
		*this->out << (this->outFirst ? "" : ", ") << "{ ";
		this->outFirst = true;
	}

	//serialize the value to string
	value.applySerializerDef(*this);

	//close bracket for objects
	if (this->action == SERIALIZER_STRINGIFY)
		*this->out << (this->outFirst ? "" : " ") << "}";

	//move
	this->outFirst = false;
}

/****************************************************/
/**
 * Pop a value from the deserialize. This is just a wrapper on the apply
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
 * Make possible to use the stream operator to deserialize (similar to pop()).
 * @param out Define the variable in which to apply the deserialization.
 * @param deserializer Define the deserializer to use.
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
