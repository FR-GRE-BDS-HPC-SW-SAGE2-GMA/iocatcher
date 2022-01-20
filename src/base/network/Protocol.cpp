/*****************************************************
*  PROJECT  : IO Catcher                             *
*  LICENSE  : Apache 2.0                             *
*  COPYRIGHT: 2020-2022 Bull SAS All rights reserved *
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
	return a.addr == b.addr && a.key == b.key;
}

/****************************************************/
bool IOC::operator==(const LibfabricObjectId & a, const LibfabricObjectId & b)
{
	return a.low == b.low && a.high == b.high;
}

/****************************************************/
void Iov::applySerializerDef(SerializerBase & serializer)
{
	serializer.apply("addr", this->addr);
	serializer.apply("key", this->key);
}

/****************************************************/
void LibfabricPing::applySerializerDef(SerializerBase & serializer)
{
	serializer.apply("rdmaSize", this->rdmaSize);
	serializer.apply("eagerSize", this->eagerSize);
	serializer.apply("rdmaIov", this->rdmaIov);
	if (eagerSize > 0)
		serializer.serializeOrPoint("eagerData", this->eagerData, this->eagerSize);
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
	this->optionalDataFragments = NULL;
	this->optionalDataFragmentCount = 0;
}

/****************************************************/
void LibfabricResponse::applySerializerDef(SerializerBase & serializer)
{
	serializer.apply("msgDataSize", this->msgDataSize);
	serializer.apply("status", this->status);
	serializer.apply("msgHasData", this->msgHasData);
	if (this->msgHasData) {
		if ((serializer.getAction() == SERIALIZER_PACK || serializer.getAction() == SERIALIZER_SIZE) && this->optionalDataFragments != NULL) {
			assert(this->optionalData == NULL);
			for (uint64_t i = 0 ; i < this->optionalDataFragmentCount ; i++)
				serializer.apply("optionalDataFragment", this->optionalDataFragments[i].buffer, this->optionalDataFragments[i].size);
		} else {
			serializer.serializeOrPoint("optionalData", this->optionalData, this->msgDataSize);
		}
	}
}

/****************************************************/
void LibfabricFirstHandshake::applySerializerDef(SerializerBase & serializer)
{
	serializer.apply("protocolVersion", this->protocolVersion);
	serializer.apply("assignLfClientId", this->assignLfClientId);
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
