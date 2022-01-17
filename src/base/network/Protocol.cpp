/*****************************************************
			 PROJECT  : IO Catcher
			 LICENSE  : Apache 2.0
			COPYRIGHT: 2020 Bull SAS
			 COPYRIGHT: 2020 Bull SAS
*****************************************************/

/****************************************************/
//internal
#include "Serializer.hpp"
#include "Protocol.hpp"

/****************************************************/
using namespace IOC;

/****************************************************/
std::ostream & IOC::operator <<(std::ostream & out, const LibfabricObjectId & objectId)
{
	out << objectId.high << ':' << objectId.low;
	return out;
}

/****************************************************/
std::ostream & operator <<(std::ostream & out, const Iov & iov)
{
	out << iov.addr << ':' << iov.key;
	return out;
}

/****************************************************/
bool IOC::operator==(const Iov & a, const Iov & b)
{
	return a.addr == a.addr && a.key == a.key;
}

/****************************************************/
bool IOC::operator==(const LibfabricObjectId & a, const LibfabricObjectId & b)
{
	return a.low == a.low && a.high == a.high;
}

/****************************************************/
void Iov::applySerializerDef(SerializerBase & serializer)
{
	serializer.apply("addr", this->addr);
	serializer.apply("key", this->key);
}

/****************************************************/
void LibfabricObjectId::applySerializerDef(SerializerBase & serializer)
{
	serializer.apply("low", this->low);
	serializer.apply("high", this->high);
}

/****************************************************/
void LibfabricMessageHeader::applySerializerDef(SerializerBase & serializer)
{
	serializer.apply("msgType", this->msgType);
	serializer.apply("lfClientId", this->lfClientId);
	serializer.apply("tcpClientId", this->tcpClientId);
	serializer.apply("tcpClientKey", this->tcpClientKey);
}

/****************************************************/
void LibfabricObjReadWriteInfos::applySerializerDef(SerializerBase & serializer)
{
	serializer.apply("objectId", this->objectId);
	serializer.apply("iov", this->iov);
	serializer.apply("offset", this->offset);
	serializer.apply("size", this->size);
	serializer.apply("msgHasData", this->msgHasData);
	if (this->msgHasData)
		serializer.serializeOrPoint("optionalData", this->optionalData, this->size);
}

/****************************************************/
void LibfabricObjFlushInfos::applySerializerDef(SerializerBase & serializer)
{
	serializer.apply("objectId", this->objectId);
	serializer.apply("objectoffsetId", this->offset);
	serializer.apply("size", this->size);
}

/****************************************************/
void LibfabricObjCreateInfos::applySerializerDef(SerializerBase & serializer)
{
	serializer.apply("objectId", this->objectId);
}

/****************************************************/
void LibfabricRegisterRange::applySerializerDef(SerializerBase & serializer)
{
	serializer.apply("objectId", this->objectId);
	serializer.apply("offset", this->offset);
	serializer.apply("size", this->size);
	serializer.apply("write", this->write);
}

/****************************************************/
void LibfabricUnregisterRange::applySerializerDef(SerializerBase & serializer)
{
	serializer.apply("objectId", this->objectId);
	serializer.apply("offset", this->offset);
	serializer.apply("size", this->size);
	serializer.apply("id", this->id);
	serializer.apply("write", this->write);
}

/****************************************************/
void LibfabricObjectCow::applySerializerDef(SerializerBase & serializer)
{
	serializer.apply("sourceObjectId", this->sourceObjectId);
	serializer.apply("destObjectId", this->destObjectId);
	serializer.apply("allowExist", this->allowExist);
	serializer.apply("rangeOffset", this->rangeOffset);
	serializer.apply("rangeSize", this->rangeSize);
}

/****************************************************/
void LibfabricResponse::initStatusOnly(int32_t status)
{
	this->msgDataSize = 0;
	this->optionalData = NULL;
	this->status = status;
	this->msgHasData = false;
}

/****************************************************/
void LibfabricResponse::applySerializerDef(SerializerBase & serializer)
{
	serializer.apply("msgDataSize", this->msgDataSize);
	serializer.apply("status", this->status);
	serializer.apply("msgHasData", this->msgHasData);
	if (this->msgHasData)
		serializer.serializeOrPoint("optionalData", this->optionalData, this->msgDataSize);
}

/****************************************************/
void LibfabricFirstHandshake::applySerializerDef(SerializerBase & serializer)
{
	serializer.apply("protocolVersion", this->protocolVersion);
}

/****************************************************/
void LibfabricFirstClientMessage::applySerializerDef(SerializerBase & serializer)
{
	serializer.apply("addr", this->addr, sizeof(this->addr));
}

/****************************************************/
void LibfabricEmpty::applySerializerDef(SerializerBase & serializer)
{
}

/****************************************************/
void LibfabricErrorMessage::applySerializerDef(SerializerBase & serializer)
{
	serializer.apply("errorMessage", this->errorMessage);
}
