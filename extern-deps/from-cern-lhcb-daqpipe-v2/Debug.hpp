/*****************************************************
             PROJECT  : lhcb-daqpipe
             VERSION  : 2.5.0-dev
             DATE     : 12/2017
             AUTHOR   : Valat Sébastien - CERN
             LICENSE  : CeCILL-C
*****************************************************/

/********************  INFO   ***********************/
/**
 * This file is imported from project MALT developped by Sebastien Valat.
 * at exascale lab / university of versailles
**/

#ifndef DAQ_DEBUG_HPP
#define DAQ_DEBUG_HPP

/********************  HEADERS  *********************/
#include <map>
#include "FormattedMessage.hpp"

/*******************  NAMESPACE  ********************/
namespace DAQ
{

/*********************  ENUM  ***********************/
/**
 * @brief Define the debug message levels.
**/
enum DebugLevel
{
	/** For assertions, but your might use standard assert() instead. **/
	MESSAGE_ASSERT,
	/** Used to print some debug informations. **/
	MESSAGE_DEBUG,
	/** Used to print some informations. **/
	MESSAGE_INFO,
	/** Used for normal printing of formatted messages. **/
	MESSAGE_NORMAL,
	/** Used to print warnings. **/
	MESSAGE_WARNING,
	/** Used to print errors. **/
	MESSAGE_ERROR,
	/** Used to print fatal error and abort(). **/
	MESSAGE_FATAL
};

/*********************  TYPES  **********************/
/**
 * Permit to enable of disable debug messages by category.
**/
typedef std::map<std::string,bool> DebugCategoryMap;

/*********************  CLASS  **********************/
/**
 * @brief Base class to manage errors.
 * 
 * The debug class provide a short way to format error, debug or warning messages and
 * manage error final state (abort, exception...).
 * 
@code{.cpp}
Debug("My message as ard %1 and %2 !",MESSAGE_FATAL).arg(value1).arg(value2).end();
@endcode
**/
class Debug : public FormattedMessage
{
	public:
		Debug(const char * format,const char * file,int line,DebugLevel level = MESSAGE_DEBUG, const char * category = nullptr);
		Debug(const char * format,DebugLevel level = MESSAGE_DEBUG,const char * category = nullptr);
		virtual ~Debug();
		virtual void end();
		static void enableCat(const std::string & cat);
		static void enableAll();
		static bool showCat(const char * cat);
		static void showList();
		static void setVerbosity(const std::string & value);
		static const DebugCategoryMap* getCatMap();
		static void setRank(int rank);
	protected:
		/** Category (only for DEBUG level), nullptr otherwise. **/
		const char * cat;
		/** Store the message level for end() method. **/
		DebugLevel level;
		/** Store file location to inform user in end() method. **/
		std::string file;
		/** Store line to inform user in end() method. **/
		int line;
		/** To check is message is emitted at exit (only for debug purpose). **/
		bool emitted;
		/** map to enable or disable messages depending on their category. **/
		static DebugCategoryMap * catMap;
		/** Store the maxmium number of chat of categories. **/
		static int catMaxWidth;
		/** Store current rank to prefix output **/
		static int rank;
};

/*********************  CLASS  **********************/
/**
 * Provide the same interface than Debug class but do nothing in implementation.
 * It is used to ignore debug message at compile time if NDEBUG macro is enabled.
**/
class DebugDummy
{
	public:
		DebugDummy(const char * format,const char * file,int line,DebugLevel level = MESSAGE_DEBUG, const char * category = nullptr){};
		DebugDummy(const char * format,DebugLevel level = MESSAGE_DEBUG,const char * category = nullptr){};
		static void enableCat(const std::string & cat){};
		static void enableAll(){};
		static bool showCat(const char * cat){return false;};
		static void showList(){};
		static void setVerbosity(const std::string & value){};
		//from message class
		template <class T> DebugDummy & arg(const T & value) {return *this;}
		DebugDummy & arg(const std::string & value) {return *this;};
		DebugDummy & arg(const char * value) {return *this;};
		DebugDummy & argStrErrno() {return *this;};
		DebugDummy & argUnit(unsigned long value,const char * unit = "") {return *this;};
		DebugDummy & argUnit1000(unsigned long value,const char * unit = "") {return *this;};		
		DebugDummy & argUnit1024(unsigned long value,const char * unit = "") {return *this;};
		std::string toString() const {return "";};
		void toStream(std::ostream & out) const {};
		void end(){};
};

/********************  MACROS  **********************/
/**
 * Helper to get current location (file and line).
**/
#define DAQ_CODE_LOCATION __FILE__,__LINE__

/*******************  FUNCTION  *********************/
/** Short function to print debug static messages without arguments. **/
inline Debug debug(const char * format, const char * cat)   {return Debug(format,MESSAGE_DEBUG,cat);  }
/** Short function to print warning static messages without arguments. **/
inline Debug warning(const char * format) {return Debug(format,MESSAGE_WARNING);}
/** Short function to print error static messages without arguments. **/
inline Debug error(const char * format)   {return Debug(format,MESSAGE_ERROR);  }
/** Short function to print fatal static messages without arguments. **/
inline Debug fatal(const char * format)   {return Debug(format,MESSAGE_FATAL);  }

/********************  MACROS  **********************/
#define DAQ_FATAL(x)     DAQ_FATAL_ARG(x).end()
#define DAQ_DEBUG(cat,x) DAQ_DEBUG_ARG((cat),(x)).end()
#define DAQ_ERROR(x)     DAQ_ERROR_ARG(x).end()
#define DAQ_WARNING(x)   DAQ_WARNING_ARG(x).end()
#define DAQ_MESSAGE(x)   DAQ_MESSAGE_ARG(x).end()
#define DAQ_INFO(x)      DAQ_INFO_ARG(x).end()

/********************  MACROS  **********************/
#define DAQ_FATAL_ARG(x)     DAQ::Debug(x,DAQ_CODE_LOCATION,DAQ::MESSAGE_FATAL    )
#define DAQ_ERROR_ARG(x)     DAQ::Debug(x,DAQ_CODE_LOCATION,DAQ::MESSAGE_ERROR    )
#define DAQ_WARNING_ARG(x)   DAQ::Debug(x,DAQ_CODE_LOCATION,DAQ::MESSAGE_WARNING  )
#define DAQ_MESSAGE_ARG(x)   DAQ::Debug(x,DAQ_CODE_LOCATION,DAQ::MESSAGE_NORMAL   )
#define DAQ_INFO_ARG(x)      DAQ::Debug(x,DAQ_CODE_LOCATION,DAQ::MESSAGE_INFO     )
#ifdef NDEBUG
	#define DAQ_DEBUG_ARG(cat,x) DAQ::DebugDummy(x,DAQ_CODE_LOCATION,DAQ::MESSAGE_DEBUG,cat)
#else
	#define DAQ_DEBUG_ARG(cat,x) DAQ::Debug(x,DAQ_CODE_LOCATION,DAQ::MESSAGE_DEBUG,cat)
#endif

/********************  MACROS  **********************/
/**
 * Check the given condition and print message and abort if it is
 * not true. In other word this is similar to assert to with
 * an explicit message for final user and stay independently of NDEBUG.
 * @param check Condition to be checked.
 * @param message Message to print if condition is not valid.
**/
#define assume(check,message) do { if (!(check)) DAQ_FATAL(message); } while(0)
/**
 * Check the given condition and print message and abort if it is
 * not true. In other word this is similar to assert to with
 * an explicit message for final user and stay independently of NDEBUG.
 * This variant provide a format which can be replaced by latter
 * calls to .arg().
 *
 * \b Remark:
 * Do not format to call .end() to emit the message.
 * 
 * \b Usage:
 * @code
 * assumeArg(fp != NULL, "Fail to open file : %1 with 2%").arg(file).argStrErrno().end()
 * @endcode
 * 
 * @param check Condition to be checked.
 * @param format Message to print if condition is not valid.
**/
#define assumeArg(check,format) if (!(check)) DAQ_FATAL_ARG(format)

/********************  MACROS  **********************/
#define DAQ_TO_STRING(x) #x
#ifdef NDEBUG
	#define DAQ_ASSERT(x)      do{} while(0)
#else
	#define DAQ_ASSERT(x)      do{ if (!(x)) DAQ::Debug(DAQ_TO_STRING(x),DAQ_CODE_LOCATION,DAQ::MESSAGE_ASSERT).end(); } while(0)
#endif

}

#endif //DAQ_DEBUG_HPP
