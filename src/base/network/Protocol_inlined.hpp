/*****************************************************
*  PROJECT  : IO Catcher                             *
*  LICENSE  : Apache 2.0                             *
*  COPYRIGHT: 2020-2022 Bull SAS All rights reserved *
*****************************************************/

/****************************************************/
#ifndef IOC_PROTOCOL_INLINED_HPP
#define IOC_PROTOCOL_INLINED_HPP

/****************************************************/
//internal
#include "Serializer.hpp"
#include "Protocol.hpp"

/****************************************************/
namespace IOC
{

/****************************************************/
inline void Iov::applySerializerDef(SerializerBase & serializer)
{
	serializer.apply("addr", this->addr);
	serializer.apply("key", this->key);
}

/****************************************************/
inline void LibfabricPing::applySerializerDef(SerializerBase & serializer)
{
	serializer.apply("rdmaSize", this->rdmaSize);
	serializer.apply("eagerSize", this->eagerSize);
	serializer.apply("rdmaIov", this->rdmaIov);
	if (eagerSize > 0)
		serializer.serializeOrPoint("eagerData", this->eagerData, this->eagerSize);
}

/****************************************************/
inline void LibfabricObjectId::applySerializerDef(SerializerBase & serializer)
{
	serializer.apply("low", this->low);
	serializer.apply("high", this->high);
}

/****************************************************/
inline void LibfabricMessageHeader::applySerializerDef(SerializerBase & serializer)
{
	serializer.apply("msgType", this->msgType);
	serializer.apply("lfClientId", this->lfClientId);
	serializer.apply("tcpClientId", this->tcpClientId);
	serializer.apply("tcpClientKey", this->tcpClientKey);
}

/****************************************************/
inline void LibfabricObjReadWriteInfos::applySerializerDef(SerializerBase & serializer)
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
inline void LibfabricObjFlushInfos::applySerializerDef(SerializerBase & serializer)
{
	serializer.apply("objectId", this->objectId);
	serializer.apply("objectoffsetId", this->offset);
	serializer.apply("size", this->size);
}

/****************************************************/
inline void LibfabricObjCreateInfos::applySerializerDef(SerializerBase & serializer)
{
	serializer.apply("objectId", this->objectId);
}

/****************************************************/
inline void LibfabricRegisterRange::applySerializerDef(SerializerBase & serializer)
{
	serializer.apply("objectId", this->objectId);
	serializer.apply("offset", this->offset);
	serializer.apply("size", this->size);
	serializer.apply("write", this->write);
}

/****************************************************/
inline void LibfabricUnregisterRange::applySerializerDef(SerializerBase & serializer)
{
	serializer.apply("objectId", this->objectId);
	serializer.apply("offset", this->offset);
	serializer.apply("size", this->size);
	serializer.apply("id", this->id);
	serializer.apply("write", this->write);
}

/****************************************************/
inline void LibfabricObjectCow::applySerializerDef(SerializerBase & serializer)
{
	serializer.apply("sourceObjectId", this->sourceObjectId);
	serializer.apply("destObjectId", this->destObjectId);
	serializer.apply("allowExist", this->allowExist);
	serializer.apply("rangeOffset", this->rangeOffset);
	serializer.apply("rangeSize", this->rangeSize);
}

/****************************************************/
inline void LibfabricResponse::initStatusOnly(int32_t status)
{
	this->msgDataSize = 0;
	this->optionalData = NULL;
	this->status = status;
	this->msgHasData = false;
	this->optionalDataFragments = NULL;
	this->optionalDataFragmentCount = 0;
}

/****************************************************/
inline void LibfabricResponse::applySerializerDef(SerializerBase & serializer)
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
inline void LibfabricFirstHandshake::applySerializerDef(SerializerBase & serializer)
{
	serializer.apply("protocolVersion", this->protocolVersion);
	serializer.apply("assignLfClientId", this->assignLfClientId);
}

/****************************************************/
inline void LibfabricFirstClientMessage::applySerializerDef(SerializerBase & serializer)
{
	serializer.apply("addr", this->addr, sizeof(this->addr));
}

/****************************************************/
inline void LibfabricEmpty::applySerializerDef(SerializerBase & serializer)
{
}

/****************************************************/
inline void LibfabricErrorMessage::applySerializerDef(SerializerBase & serializer)
{
	serializer.apply("errorMessage", this->errorMessage);
}

}

#endif //IOC_PROTOCOL_INLINED_HPP