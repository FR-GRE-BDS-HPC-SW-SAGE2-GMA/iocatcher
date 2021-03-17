/*****************************************************
			 PROJECT  : IO Catcher
			 LICENSE  : Apache 2.0
			COPYRIGHT: 2020 Bull SAS
			 COPYRIGHT: 2020 Bull SAS
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
