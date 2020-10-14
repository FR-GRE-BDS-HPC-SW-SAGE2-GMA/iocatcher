/*****************************************************
			 PROJECT  : IO Catcher
			 LICENSE  : Apache 2.0
			COPYRIGHT: 2020 Bull SAS
			 COPYRIGHT: 2020 Bull SAS
*****************************************************/

/** This file provide the glue to call daqpipe debug functions **/

#ifndef UMMAP_DEBUG_HPP
#define UMMAP_DEBUG_HPP

/********************  HEADERS  *********************/
#include <from-cern-lhcb-daqpipe-v2/Debug.hpp>

/********************  NAMESPACE  *******************/
/**
 * Namespace containing all sources for UMMAP-IO-V2
**/
namespace ummapio
{

/********************  MACROS  **********************/
/**
 * Display the given message and abort.
 * @param message Message to print.
**/
#define UMMAP_FATAL(message) DAQ_FATAL(message)
/**
 * Display the given message under the given category. It will
 * be displayed only if the category filter is enabled.
 * @param cat The category of the message.
 * @param message The message to print.
**/
#define UMMAP_DEBUG(cat,message) DAQ_DEBUG(cat,x)
/**
 * Display the given error message and pursue.
 * @param message Message to display.
**/
#define UMMAP_ERROR(message) DAQ_ERROR(message)
/**
 * Display the given warning message and pursue.
 * @param message Message to display.
**/
#define UMMAP_WARNING(message) DAQ_WARNING(message)
/**
 * Display the given message.
 * @param message Message to display
**/
#define UMMAP_MESSAGE(message) DAQ_MESSAGE(message)
/**
 * Display the info message.
 * @param message Message to display.
**/
#define UMMAP_INFO(message) DAQ_INFO(message)

/********************  MACROS  **********************/
/**
 * Display the given message and abort.
 * 
 * \b Remark:
 * Do not format to call .end() to emit the message.
 * 
 * \b Usage:
 * @code
 * UMMAP_FATAL_ARG("Message saying %1").arg("hello").end()
 * @endcode
 * 
 * @param format Message to print. %1, %2.... 
 * values will be replaced by the .arg() latter calls.
**/
#define UMMAP_FATAL_ARG(format) DAQ_FATAL_ARG(format)
/**
 * Display the given error message.
 * 
 * \b Remark:
 * Do not format to call .end() to emit the message.
 * 
 * \b Usage:
 * @code
 * UMMAP_ERROR_ARG("Message saying %1").arg("hello").end()
 * @endcode
 * 
 * @param format Message to print. %1, %2.... 
 * values will be replaced by the .arg() latter calls.
**/
#define UMMAP_ERROR_ARG(format) DAQ_ERROR_ARG(format)
/**
 * Display the given warning message.
 * 
 * \b Remark:
 * Do not format to call .end() to emit the message.
 * 
 * \b Usage:
 * @code
 * UMMAP_WARNING_ARG("Message saying %1").arg("hello").end()
 * @endcode
 * 
 * @param format Message to print. %1, %2.... 
 * values will be replaced by the .arg() latter calls.
**/
#define UMMAP_WARNING_ARG(format) DAQ_WARNING_ARG(format)
/**
 * Display the given message.
 * 
 * \b Remark:
 * Do not format to call .end() to emit the message.
 * 
 * \b Usage:
 * @code
 * UMMAP_MESSAGE_ARG("Message saying %1").arg("hello").end()
 * @endcode
 * 
 * @param format Message to print. %1, %2.... 
 * values will be replaced by the .arg() latter calls.
**/
#define UMMAP_MESSAGE_ARG(format) DAQ_MESSAGE_ARG(format)
/**
 * Display the given info message.
 * 
 * \b Remark:
 * Do not format to call .end() to emit the message.
 * 
 * \b Usage:
 * @code
 * UMMAP_MESSAGE_ARG("Message saying %1").arg("hello").end()
 * @endcode
 * 
 * @param format Message to print. %1, %2.... 
 * values will be replaced by the .arg() latter calls.
**/
#define UMMAP_INFO_ARG(format) DAQ_INFO_ARG(format)
/**
 * Display the given message under the given category. It will
 * be displayed only if the category filter is enabled.
 * 
 * \b Remark:
 * Do not format to call .end() to emit the message.
 * 
 * \b Usage:
 * @code
 * UMMAP_MESSAGE_ARG("Message saying %1").arg("hello").end()
 * @endcode
 * 
 * @param cat The category of the message.
 * @param format Message to print. %1, %2.... 
 * values will be replaced by the .arg() latter calls.
**/
#define UMMAP_DEBUG_ARG(cat,format) DAQ_DEBUG_ARG(cat,format)

/********************  MACROS  **********************/
#ifdef NDEBUG
	/** Custom internal assertion **/
	#define UMMAP_ASSERT(x)      do{} while(0)
#else
	/** Custom internal assertion **/
	#define UMMAP_ASSERT(x)      do{ if (!(x)) DAQ::Debug(DAQ_TO_STRING(x),DAQ_CODE_LOCATION,DAQ::MESSAGE_ASSERT).end(); } while(0)
#endif

}

#endif //UMMAP_DEBUG_HPP
