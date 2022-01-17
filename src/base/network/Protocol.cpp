/*****************************************************
*  PROJECT  : IO Catcher                             *
*  LICENSE  : Apache 2.0                             *
*  COPYRIGHT: 2020-2022 Bull SAS All rights reserved *
*****************************************************/

/****************************************************/
//internal
#include "Protocol.hpp"

/****************************************************/
using namespace IOC;

/****************************************************/
std::ostream & IOC::operator <<(std::ostream & out, const LibfabricObjectId & objectId)
{
	out << objectId.high << ':' << objectId.low;
	return out;
}
