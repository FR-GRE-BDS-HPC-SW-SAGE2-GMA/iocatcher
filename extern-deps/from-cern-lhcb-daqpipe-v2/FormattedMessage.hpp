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

#ifndef DAQ_FORMALTED_MESSAGE_HPP
#define DAQ_FORMALTED_MESSAGE_HPP

/********************  HEADERS  *********************/
//standard
#include <vector>
#include <string>
#include <sstream>

/*******************  NAMESPACE  ********************/
namespace DAQ
{

/*********************  TYPES  **********************/
/**
 * @brief Vector to store the values to replace into the formatted message. 
**/
typedef std::vector<std::string> FormattedMessageEntries;
	
/*********************  CLASS  **********************/
/**
 * Provide a short class to use formatted message with full C++ stream operators support
 * and avoid to manually manage local fixed size buffers to use sprintf. Similar usage
 * can be found into QT translation system, thanks to them for the idea.
 * 
 * @brief Short class to format messages with object support.
**/
class FormattedMessage
{
	public:
		FormattedMessage(const char * format);
		FormattedMessage(const std::string & format);
		virtual ~FormattedMessage() = default;
		template <class T> FormattedMessage & arg(const T & value);
		FormattedMessage & arg(const std::string & value);
		FormattedMessage & arg(const char * value);
		FormattedMessage & argStrErrno();
		FormattedMessage & argUnit1000(unsigned long value,const char * unit = "", int staticUnit = -1);
		FormattedMessage & argUnit1024(unsigned long value,const char * unit = "", int staticUnit = -1);
		std::string toString() const;
		void toStream(std::ostream & out) const;
		virtual void end();
	public:
		friend std::ostream & operator << (std::ostream & out,const FormattedMessage & message);
	protected:
		void pushValue(std::ostream & out,int id) const;
	private:
		/** Store the string format **/
		std::string format;
		/** Store the entries to replace into format into toString() method. **/
		FormattedMessageEntries entries;
};

/*******************  FUNCTION  *********************/
/**
 * Setup argument value. First call will setup %1, second %2... All
 * values are converted to string by standard STL stream operator onto stringstream object.
 * @param value Value to convert to string and to setup as argument value.
**/
template <class T>
FormattedMessage& FormattedMessage::arg(const T& value)
{
	//convert
	std::stringstream buffer;
	buffer << value;
	
	//insert
	entries.push_back(buffer.str());
	
	return *this;
}

}

#endif //DAQ_FORMALTED_MESSAGE_HPP
