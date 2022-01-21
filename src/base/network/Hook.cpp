/*****************************************************
			PROJECT  : IO Catcher
			VERSION  : 0.0.0-dev
			DATE     : 10/2020
			LICENSE  : ????????
*****************************************************/

/****************************************************/
#include "Hook.hpp"
#include "LibfabricConnection.hpp"

/****************************************************/
using namespace IOC;

/****************************************************/
/**
 * Terminate the request handling and repost the recive buffer.
**/
void LibfabricClientRequest::terminate(void)
{
	this->connection->repostReceive(*this);
}
