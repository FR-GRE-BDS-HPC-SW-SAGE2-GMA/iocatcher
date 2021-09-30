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

/********************  HEADERS  *********************/
//as first
#include "config.h"
//standard
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <cstring>
#include <cstdlib>
//header to implement
#include "Debug.hpp"

/*******************  NAMESPACE  ********************/
namespace DAQ
{

/********************  MACRO  ***********************/
#define COLOR_NORMAL   "\033[0m"
#define COLOR_RED      "\033[31m"
#define COLOR_GREEN    "\033[32m"
#define COLOR_YELLOW   "\033[33m"
#define COLOR_BLUE     "\033[34m"
#define COLOR_MAGENTA  "\033[35m"
#define COLOR_CYAN     "\033[36m"
#define COLOR_WHITE    "\033[37m"

/********************** CONSTS **********************/
/**
 * String to print names of debug levels.
**/
#ifdef ENABLE_COLOR
	static const char * cstLevelPrefix[] = {COLOR_RED "Assert : ",COLOR_CYAN , COLOR_CYAN,"", COLOR_YELLOW "Warning : ", COLOR_RED"Error : ",COLOR_RED"Fatal : "};
	static const char * cstPostfix = COLOR_NORMAL;
#else
	static const char * cstLevelPrefix[] = {"Assert : ","Debug : ","Info : ","","Warning : ","Error : ","Fatal : "};
	static const char * cstPostfix = "";
#endif

/********************  GLOBALS  *********************/
static DebugCategoryMap ref;
DebugCategoryMap * Debug::catMap = &ref;
int Debug::catMaxWidth = 0;
int Debug::rank = -1;
bool Debug::disabled = true;
std::function<void(const std::string& message)> Debug::beforeAbort;

/*******************  FUNCTION  *********************/
/**
 * Constructor of debug object. Arguments must be setup with arg() method and message printing
 * will be effective when calling the end() method.
 * @param format Define the message format (use %X for aguments where X is the ID of arg).
 * @param file Define the source location of the message emitter.
 * @param line Define the source location of the message emitter.
 * @param level Define the level of debug message.
 * @param cat Optional category of message to filter output.
 * 
@code
Debug msg("this is a test with arg %1 and arg %2 !").arg(10).arg("string").end();
@endcode
**/
Debug::Debug(const char* format, const char* file, int line, DebugLevel level, const char * cat)
	:FormattedMessage(format)
{
	this->cat = cat;
	this->file = file;
	this->line = line;
	this->level = level;
	this->emitted = false;
}

/*******************  FUNCTION  *********************/
/**
 * Short constructor without code location.
**/
Debug::Debug(const char* format, DebugLevel level, const char * category)
	:FormattedMessage(format)
{
	this->cat = category;
	this->file = "??";
	this->line = 0;
	this->emitted = false;
	this->level = level;
}

/*******************  FUNCTION  *********************/
/**
 * Set global variable rank to be used when printing to know from which node the output comes.
 * @param rank Define the current rank value to setup.
**/
void Debug::setRank(int rank)
{
	Debug::rank = rank;
}

/*******************  FUNCTION  *********************/
/**
 * Emit the message.
**/
void Debug::end()
{
	this->emitted = true;
	std::stringstream buf;
	
	//select
	switch(level) {
		case MESSAGE_DEBUG:
			#ifndef NDEBUG
				if (showCat(cat))
				{
					//buf << cstLevelPrefix[level] << '[' << std::setw(3) << std::setfill('0') << rank << std::setfill(' ') <<  "] " << '[' << std::left << std::setw(catMaxWidth) << cat << "] " << *this << cstPostfix << std::endl;
					buf << cstLevelPrefix[level] << '[' << std::left << std::setw(catMaxWidth) << cat << "] " << *this << cstPostfix << std::endl;
					std::cout << buf.str();
				}
			#endif //NDEBUG
			break;
		case MESSAGE_INFO:
		case MESSAGE_NORMAL:
			///if (line != 0)
			//	std::cout << std::endl << cstLevelPrefix[level] << "At " <<  file << ':' << line << " : \n";
			//buf << cstLevelPrefix[level] << '[' << std::setw(3) << std::setfill('0') << rank << std::setfill(' ') << "] " << *this << cstPostfix << std::endl;
			buf << cstLevelPrefix[level] << *this << cstPostfix << std::endl;
			std::cout << buf.str();
			break;
		case MESSAGE_ASSERT:
			#ifdef NDEBUG
				break;
			#else
				if (line != 0)
					buf << std::endl << cstLevelPrefix[level] << "At " <<  file << ':' << line << " : \n";
				//buf << cstLevelPrefix[level] << '[' << std::setw(3) << std::setfill('0') << rank << std::setfill(' ') << "] " << *this << cstPostfix << std::endl;
				buf << cstLevelPrefix[level] << *this << cstPostfix << std::endl;
				std::cout << buf.str();
				if (beforeAbort)
					beforeAbort(buf.str());
				abort();
			#endif
		case MESSAGE_WARNING:
		case MESSAGE_ERROR:
			if (line != 0)
				buf << std::endl << cstLevelPrefix[level] << "At " <<  file << ':' << line << " : \n";
			//buf << cstLevelPrefix[level] << '[' << std::setw(3) << std::setfill('0') << rank << std::setfill(' ') << "] " << *this << cstPostfix << std::endl;
			buf << cstLevelPrefix[level] << *this << cstPostfix << std::endl;
			std::cerr << buf.str();
			break;
		case MESSAGE_FATAL:
			if (line != 0)
				buf << std::endl << cstLevelPrefix[level] << "At " <<  file << ':' << line << " : \n";
			//buf << cstLevelPrefix[level] << '[' << std::setw(3) << std::setfill('0') << rank << std::setfill(' ') << "] " << *this << cstPostfix << std::endl;
			buf << cstLevelPrefix[level] << *this << cstPostfix << std::endl;
			std::cerr << buf.str();
			if (beforeAbort)
				beforeAbort(buf.str());
			//check exit mode. In debug mode we abort by default
			//in production we depend on env var not to generate big core dump if not wanted.
			#ifdef NDEBUG
				const char * abortEnv = getenv("IOC_ABORT");
				if ( abortEnv != NULL && (strncmp("true", abortEnv, 4) == 0 || strncmp("yes", abortEnv, 3) == 0))
					abort();
				else
					exit(EXIT_FAILURE);
			#else
				abort();
			#endif
			break;
	}
}

/*******************  FUNCTION  *********************/
/**
 * Destructor, only to check message emission on debug mode.
**/
Debug::~Debug()
{
	assert(emitted);
}

/*******************  FUNCTION  *********************/
/**
 * Enable display of messages from selected categoty.
 * @param cat Select the category of message to display.
**/
void Debug::enableCat ( const std::string& cat )
{
	//setup
	if (catMap == nullptr)
		catMap = new DebugCategoryMap;
	
	//set length
	int len = cat.size();
	if (len > catMaxWidth)
		catMaxWidth = len;
	
	//set
	(*catMap)[cat] = true;

	//enable debug messages
	disabled = false;
}

/*******************  FUNCTION  *********************/
/**
 * Check if the given category has to be shown or not.
 * @param cat Name of the categoty to test.
**/
bool Debug::showCat ( const char* cat )
{
	if (catMap == nullptr) {
		int len = strlen(cat);
		if (len > catMaxWidth)
			catMaxWidth = len;
	}

	if (catMap == nullptr || cat == nullptr) {
		return true;
	} else {
		DebugCategoryMap::const_iterator it = catMap->find(cat);
		if (it == catMap->end()) {
			bool parentStatus = showParentsCat(cat);
			(*catMap)[cat] = parentStatus;
			return parentStatus;
		} else {
			return it->second;
		}
	}
}

/*******************  FUNCTION  *********************/
bool Debug::showParentsCat(const char * cat)
{
	//trivial
	if (cat == NULL)
		return false;

	//check
	if (strlen(cat) >= 64) {
		DAQ_FATAL_ARG("Category name is too long and overpass the 64 character limit: '%1' !")
			.arg(cat)
			.end();
	}
	
	//loop until end
	char buffer[64];
	char * bufferCursor = buffer;
	while(*cat != '\0'){
		//go until end or : or .
		while(*cat != '\0' && *cat != ':' && *cat != '.')
			*(bufferCursor++) = *(cat++);

		//mark end
		*bufferCursor = '\0';
		
		//if separator check if parent is enabled
		if (*cat == ':' || *cat == '.') {
			if (catMap->find(buffer) != catMap->end())
				return true;
			else
				*(bufferCursor++) = *(cat++);
		} else {
			return false;
		}
	}

	//not enabled
	return false;
}

/*******************  FUNCTION  *********************/
/**
 * Enable all category to be shown by setting catMap to nullptr.
**/
void Debug::enableAll ()
{
	catMap = nullptr;
	disabled = false;
}

/*******************  FUNCTION  *********************/
/**
 * Can help user by printing all the available categories at execution end.
 * Enabled by using '-v help'.
**/
void Debug::showList ()
{
	std::string buffer = "Debug categories :\n";
	if (catMap != nullptr) {
		for (DebugCategoryMap::const_iterator it = catMap->begin() ; it != catMap->end() ; ++it) {
			buffer += " - ";
			buffer += it->first;
			buffer += "\n";
		}
	}
	DAQ_INFO(buffer.c_str());
}

/*******************  FUNCTION  *********************/
void Debug::setBeforeAbortHandler(std::function<void(const std::string& message)> handler)
{
	Debug::beforeAbort = handler;
}

/*******************  FUNCTION  *********************/
/**
 * Enable verbosity for the given categories listed as a string with
 * comma separated values.
 * @param value A string with comma separated categorie names.
**/
void Debug::setVerbosity ( const std::string& value )
{
	std::istringstream iss(value);
	std::string token;
	while(std::getline(iss, token, ',')) {
		if (token == "all" || token == "*")
			enableAll();
		else
			enableCat(token);
	}
}

/*******************  FUNCTION  *********************/
const DebugCategoryMap * Debug::getCatMap ()
{
	return catMap;
}

/********************  GLOBALS  *********************/
void Debug::initLoadEnv(void)
{
	const char * env = getenv("IOC_DEBUG");
	if (env != NULL) {
		Debug::catMap = new DebugCategoryMap;
		DAQ_INFO("Enabling debugging logs via IOC_DEBUG environnement variable !");
		Debug::setVerbosity(env);
	}
}

/********************  GLOBALS  *********************/
/**
 * We use a class constructor to be init after stdc++ otherwise if we use
 * c lib constructor we arrive too soon which make crashing when using
 * some objects.
**/
class DebugInitializer {
	public:
		DebugInitializer(void) {
			Debug::initLoadEnv();
		}
};
static DebugInitializer gblDebugInitializer;

}
