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
