/*****************************************************
			PROJECT  : IO Catcher
			LICENSE  : Apache 2.0
			COPYRIGHT: 2020 Bull SAS
*****************************************************/

/****************************************************/
#include "HookLambdaFunction.hpp"

/****************************************************/
using namespace IOC;

/****************************************************/
/**
 * Constructor.
 * @param function The lambda function to assign.
**/
HookLambdaFunction::HookLambdaFunction(HookLambdaDef function)
{
	this->function = function;
}

/****************************************************/
/** Destructor **/
HookLambdaFunction::~HookLambdaFunction(void)
{
}

/****************************************************/
//inherit
LibfabricActionResult HookLambdaFunction::onMessage(LibfabricConnection * connection, uint64_t lfClientId, size_t msgBufferId, LibfabricMessage * clientMessage)
{
	return this->function(connection, lfClientId, msgBufferId, clientMessage);
}
