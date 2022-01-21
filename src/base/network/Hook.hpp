/*****************************************************
*  PROJECT  : IO Catcher                             *
*  LICENSE  : Apache 2.0                             *
*  COPYRIGHT: 2020-2022 Bull SAS All rights reserved *
*****************************************************/

#ifndef IOC_HOOK_HPP
#define IOC_HOOK_HPP

/****************************************************/
#include "Serializer.hpp"
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
 * Structure used to return the message info from pollMessage.
**/
struct LibfabricClientRequest
{
	/** Terminate the request and repost the buffer.**/
	void terminate(void);
	/** ID of the client which sends the message. **/
	uint64_t lfClientId;
	/** Message buffer to be returned to the connection after using the data.**/
	size_t msgBufferId;
	/** Header of the client message containing the protocol informations. **/
	LibfabricMessageHeader header;
	/** Keep track of the connection handling the request.**/
	LibfabricConnection * connection;
	/** The deserialization tool. **/
	DeSerializer deserializer;
};

/****************************************************/
typedef LibfabricClientRequest LibfabricRemoteResponse;

/****************************************************/
/**
 * Signature of a hook function to avoid duplicating the signature at many places.
**/
typedef std::function<LibfabricActionResult(LibfabricConnection * connection, LibfabricClientRequest & request)> HookLambdaDef;

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
		 * @param request Contain all the request informations (client identification & message & buffer ID).
		 * @return It returns an enum saying if the polling loop need to exit or not.
		**/
		virtual LibfabricActionResult onMessage(LibfabricConnection * connection, LibfabricClientRequest & request) = 0;
};

}

#endif //IOC_HOOK_HPP
