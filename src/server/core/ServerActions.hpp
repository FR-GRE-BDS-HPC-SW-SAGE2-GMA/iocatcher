/*****************************************************
            PROJECT  : IO Catcher
            LICENSE  : Apache 2.0
			COPYRIGHT: 2020 Bull SAS
COPYRIGHT: 2020 Bull SAS
*****************************************************/

#ifndef IOC_SERVER_ACTIONS_HPP
#define IOC_SERVER_ACTIONS_HPP

/****************************************************/
//linux
#include <sys/uio.h>
//local
#include "Container.hpp"
#include "../../base/network/LibfabricDomain.hpp"
#include "../../base/network/LibfabricConnection.hpp"

/****************************************************/
namespace IOC
{

/****************************************************/
//TODO work to clean this
extern bool gblConsistencyCheck;
extern size_t gblReadSize;
extern size_t gblWriteSize;

/****************************************************/
void setupPingPong(LibfabricConnection & connection);
void setupObjFlush(LibfabricConnection & connection, Container & container);
void setupObjRangeRegister(LibfabricConnection & connection, Container & container);
void setupObjUnregisterRange(LibfabricConnection & connection, Container & container);
void setupObjCreate(LibfabricConnection & connection, Container & container);
iovec * buildIovec(ObjectSegmentList & segments, size_t offset, size_t size);
void objRdmaPushToClient(LibfabricConnection & connection, int clientId, LibfabricMessage * clientMessage, ObjectSegmentList & segments);
void objEagerPushToClient(LibfabricConnection & connection, int clientId, LibfabricMessage * clientMessage, ObjectSegmentList & segments);
void setupObjRead(LibfabricConnection & connection, Container & container);
void objRdmaFetchFromClient(LibfabricConnection & connection, int clientId, LibfabricMessage * clientMessage, ObjectSegmentList & segments);
void objEagerExtractFromMessage(LibfabricConnection & connection, int clientId, LibfabricMessage * clientMessage, ObjectSegmentList & segments);
void setupObjWrite(LibfabricConnection & connection, Container & container);

}

#endif //IOC_SERVER_ACTIONS_HPP
