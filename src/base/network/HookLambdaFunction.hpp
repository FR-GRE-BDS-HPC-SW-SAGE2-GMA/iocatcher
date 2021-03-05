/*****************************************************
			PROJECT  : IO Catcher
			LICENSE  : Apache 2.0
			COPYRIGHT: 2020 Bull SAS
*****************************************************/

#ifndef IOC_HOOK_LAMBDA_FUNCTION_HPP
#define IOC_HOOK_LAMBDA_FUNCTION_HPP

/****************************************************/
#include <functional>
#include "Hook.hpp"

/****************************************************/
namespace IOC
{

typedef std::function<LibfabricActionResult(LibfabricConnection * connection, int, size_t, void*)> HookLambdaDef;

/****************************************************/
/**
 * Implement a hook redirection to a lambda function.
**/
class HookLambdaFunction : public Hook
{
	public:
		HookLambdaFunction(HookLambdaDef function);
		virtual ~HookLambdaFunction(void);
		virtual LibfabricActionResult onMessage(LibfabricConnection * connection, int lfClientId, size_t msgBufferId, LibfabricMessage * clientMessage) override;
	private:
		/** Store the lambda function to be called when recieving a message. **/
		HookLambdaDef function;
};

}

#endif //IOC_HOOK_LAMBDA_FUNCTION_HPP
