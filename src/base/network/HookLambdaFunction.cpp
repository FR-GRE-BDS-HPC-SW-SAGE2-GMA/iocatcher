/*****************************************************
*  PROJECT  : IO Catcher                             *
*  LICENSE  : Apache 2.0                             *
*  COPYRIGHT: 2020-2022 Bull SAS All rights reserved *
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
LibfabricActionResult HookLambdaFunction::onMessage(LibfabricConnection * connection, LibfabricClientRequest & request)
{
	return this->function(connection, request);
}
