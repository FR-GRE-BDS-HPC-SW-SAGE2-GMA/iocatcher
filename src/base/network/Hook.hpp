/*****************************************************
*  PROJECT  : IO Catcher                             *
*  LICENSE  : Apache 2.0                             *
*  COPYRIGHT: 2020-2022 Bull SAS All rights reserved *
*****************************************************/

#ifndef IOC_HOOK_HPP
#define IOC_HOOK_HPP

/****************************************************/
#include "Protocol.hpp"

/****************************************************/
namespace IOC
{
/****************************************************/
class LibfabricConnection;

/****************************************************/
/**
 * Possible action to return from the callbacks.
 * @brief Actions to be returned from callbacks.
**/
enum LibfabricActionResult
{
	/** The poll(true) operation on the connection can continue polling. **/
	LF_WAIT_LOOP_KEEP_WAITING = 0,
	/** The poll(true) operation on the connection return after exit of the callback. **/
	LF_WAIT_LOOP_UNBLOCK = 1,
};

/****************************************************/
/**
 * Define a hook to be used when a message arrive.
**/
class Hook
{
	public:
		Hook(void) {};
		virtual ~Hook(void) {};
		/**
		 * Function to be called when a message for the current hook arrive.
		 * @param connection The connection to be used to respond.
		 * @param lfClientid The libfaric client ID to respond to.
		 * @param msgBufferId The message buffer ID to be returned to the connection after finishing the work.
		 * @param clientMessage Pointer to the buffer containing the client message.
		 * @return It returns an enum saying if the polling loop need to exit or not.
		**/
		virtual LibfabricActionResult onMessage(LibfabricConnection * connection, uint64_t lfClientId, size_t msgBufferId, LibfabricMessage * clientMessage) = 0;
};

}

#endif //IOC_HOOK_HPP
