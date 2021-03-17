/*****************************************************
			 PROJECT  : IO Catcher
			 LICENSE  : Apache 2.0
			COPYRIGHT: 2020 Bull SAS
			 COPYRIGHT: 2020 Bull SAS
*****************************************************/

/** This file provide the glue to call daqpipe debug functions **/

#ifndef IOC_DEBUG_HPP
#define IOC_DEBUG_HPP

/********************  HEADERS  *********************/
#include <from-cern-lhcb-daqpipe-v2/Debug.hpp>

/********************  NAMESPACE  *******************/
/**
 * Namespace containing all sources for UMMAP-IO-V2
**/
namespace IOC
{

/****************************************************/
/**
 * Display the given message and abort.
 * @param message Message to print.
**/
#define IOC_FATAL(message) DAQ_FATAL(message)
/**
 * Display the given message under the given category. It will
 * be displayed only if the category filter is enabled.
 * @param cat The category of the message.
 * @param message The message to print.
**/
#define IOC_DEBUG(cat,message) DAQ_DEBUG(cat,message)
/**
 * Display the given error message and pursue.
 * @param message Message to display.
**/
#define IOC_ERROR(message) DAQ_ERROR(message)
/**
 * Display the given warning message and pursue.
 * @param message Message to display.
**/
#define IOC_WARNING(message) DAQ_WARNING(message)
/**
 * Display the given message.
 * @param message Message to display
**/
#define IOC_MESSAGE(message) DAQ_MESSAGE(message)
/**
 * Display the info message.
 * @param message Message to display.
**/
#define IOC_INFO(message) DAQ_INFO(message)

/****************************************************/
/**
 * Display the given message and abort.
 * 
 * \b Remark:
 * Do not format to call .end() to emit the message.
 * 
 * \b Usage:
 * @code
 * IOC_FATAL_ARG("Message saying %1").arg("hello").end()
 * @endcode
 * 
 * @param format Message to print. %1, %2.... 
 * values will be replaced by the .arg() latter calls.
**/
#define IOC_FATAL_ARG(format) DAQ_FATAL_ARG(format)
/**
 * Display the given error message.
 * 
 * \b Remark:
 * Do not format to call .end() to emit the message.
 * 
 * \b Usage:
 * @code
 * IOC_ERROR_ARG("Message saying %1").arg("hello").end()
 * @endcode
 * 
 * @param format Message to print. %1, %2.... 
 * values will be replaced by the .arg() latter calls.
**/
#define IOC_ERROR_ARG(format) DAQ_ERROR_ARG(format)
/**
 * Display the given warning message.
 * 
 * \b Remark:
 * Do not format to call .end() to emit the message.
 * 
 * \b Usage:
 * @code
 * IOC_WARNING_ARG("Message saying %1").arg("hello").end()
 * @endcode
 * 
 * @param format Message to print. %1, %2.... 
 * values will be replaced by the .arg() latter calls.
**/
#define IOC_WARNING_ARG(format) DAQ_WARNING_ARG(format)
/**
 * Display the given message.
 * 
 * \b Remark:
 * Do not format to call .end() to emit the message.
 * 
 * \b Usage:
 * @code
 * IOC_MESSAGE_ARG("Message saying %1").arg("hello").end()
 * @endcode
 * 
 * @param format Message to print. %1, %2.... 
 * values will be replaced by the .arg() latter calls.
**/
#define IOC_MESSAGE_ARG(format) DAQ_MESSAGE_ARG(format)
/**
 * Display the given info message.
 * 
 * \b Remark:
 * Do not format to call .end() to emit the message.
 * 
 * \b Usage:
 * @code
 * IOC_MESSAGE_ARG("Message saying %1").arg("hello").end()
 * @endcode
 * 
 * @param format Message to print. %1, %2.... 
 * values will be replaced by the .arg() latter calls.
**/
#define IOC_INFO_ARG(format) DAQ_INFO_ARG(format)
/**
 * Display the given message under the given category. It will
 * be displayed only if the category filter is enabled.
 * 
 * \b Remark:
 * Do not format to call .end() to emit the message.
 * 
 * \b Usage:
 * @code
 * IOC_MESSAGE_ARG("Message saying %1").arg("hello").end()
 * @endcode
 * 
 * @param cat The category of the message.
 * @param format Message to print. %1, %2.... 
 * values will be replaced by the .arg() latter calls.
**/
#define IOC_DEBUG_ARG(cat,format) DAQ_DEBUG_ARG(cat,format)

/****************************************************/
#ifdef NDEBUG
	/** Custom internal assertion **/
	#define IOC_ASSERT(x)      do{} while(0)
#else
	/** Custom internal assertion **/
	#define IOC_ASSERT(x)      do{ if (!(x)) DAQ::Debug(DAQ_TO_STRING(x),DAQ_CODE_LOCATION,DAQ::MESSAGE_ASSERT).end(); } while(0)
#endif

}

/****************************************************/
/**
 * Macro to check libfabric function status and stop with an error message
 * in case of failure.
**/
#define LIBFABRIC_CHECK_STATUS(call, status) \
	do { \
		if(status != 0) { \
			IOC_FATAL_ARG("Libfabric get failure on command %1 with status %2 : %3") \
				.arg(call) \
				.arg(status) \
				.arg(fi_strerror(-status))\
				.end(); \
		} \
	} while(0)

#endif //IOC_DEBUG_HPP
