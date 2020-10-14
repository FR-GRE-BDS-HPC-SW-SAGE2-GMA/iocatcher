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
#include <cerrno>
#include <gtest/gtest.h>
#include "../FormattedMessage.hpp"
#include "../Debug.hpp"

/***************** USING NAMESPACE ******************/
using namespace DAQ;

/*******************  FUNCTION  *********************/
TEST(FormattedMessage,constructor)
{
	FormattedMessage message("Basic test");
}

/*******************  FUNCTION  *********************/
TEST(FormattedMessage,basic)
{
	FormattedMessage message("Basic test");
	EXPECT_EQ("Basic test",message.toString());
}

/*******************  FUNCTION  *********************/
TEST(FormattedMessage,string)
{
	FormattedMessage message(std::string("Basic test"));
	EXPECT_EQ("Basic test",message.toString());
}

/*******************  FUNCTION  *********************/
TEST(FormattedMessage,argFloat)
{
	FormattedMessage message("value = %1");
	message.arg(10.5);
	EXPECT_EQ("value = 10.5",message.toString());
}

/*******************  FUNCTION  *********************/
TEST(FormattedMessage,argString)
{
	std::string val = "test";
	FormattedMessage message("value = %1");
	message.arg(val);
	EXPECT_EQ("value = test",message.toString());
}

/*******************  FUNCTION  *********************/
TEST(FormattedMessage,argCString)
{
	FormattedMessage message("value = %1");
	message.arg("test");
	EXPECT_EQ("value = test",message.toString());
}

/*******************  FUNCTION  *********************/
TEST(FormattedMessage,multiArg)
{
	FormattedMessage message("value = %1, %2, %3");
	message.arg(10);
	message.arg(20);
	message.arg(30.1);
	EXPECT_EQ("value = 10, 20, 30.1",message.toString());
}

/*******************  FUNCTION  *********************/
TEST(FormattedMessage,notEnought)
{
	FormattedMessage message("value = %1, %2, %3, %4");
	message.arg(10);
	message.arg(20);
	message.arg(30.1);
	EXPECT_EQ("value = 10, 20, 30.1, %4",message.toString());
}

/*******************  FUNCTION  *********************/
TEST(FormattedMessage,notTooMuch)
{
	FormattedMessage message("value = %1, %2, %3");
	message.arg(10);
	message.arg(20);
	message.arg(30.1);
	message.arg(40);
	EXPECT_EQ("value = 10, 20, 30.1",message.toString());
}

/*******************  FUNCTION  *********************/
TEST(FormattedMessage,operatorStream)
{
	FormattedMessage message("test %1");
	message.arg("test");
	
	std::stringstream out;
	out << message;
	
	EXPECT_EQ("test test",out.str());
}

/*******************  FUNCTION  *********************/
TEST(FormattedMessage,errnoToString)
{
	FormattedMessage message("test %1");
	errno=0;
	message.argStrErrno();
	
	EXPECT_EQ("test Success",message.toString());
}

/*******************  FUNCTION  *********************/
TEST(FormattedMessage,argUnit1024)
{
	FormattedMessage message("%1");
	message.argUnit1024(16,"B");
	EXPECT_EQ("16.00 B",message.toString());
}

/*******************  FUNCTION  *********************/
TEST(FormattedMessage,argUnit1024_KB)
{
	FormattedMessage message("%1");
	message.argUnit1024(2048,"B");
	EXPECT_EQ("2.00 KB",message.toString());
}

/*******************  FUNCTION  *********************/
TEST(FormattedMessage,argUnit1024_MB)
{
	FormattedMessage message("%1");
	message.argUnit1024(2048*1024,"B");
	EXPECT_EQ("2.00 MB",message.toString());
}

/*******************  FUNCTION  *********************/
TEST(FormattedMessage,argUnit1024_GB)
{
	FormattedMessage message("%1");
	message.argUnit1024(2048UL*1024UL*1024UL,"B");
	EXPECT_EQ("2.00 GB",message.toString());
}

/*******************  FUNCTION  *********************/
TEST(FormattedMessage,argUnit1024_TB)
{
	FormattedMessage message("%1");
	message.argUnit1024(2048UL*1024UL*1024UL*1024UL,"B");
	EXPECT_EQ("2.00 TB",message.toString());
}
