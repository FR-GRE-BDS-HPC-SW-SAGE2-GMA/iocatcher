/*****************************************************
            PROJECT  : IO Catcher
            LICENSE  : Apache 2.0
			COPYRIGHT: 2020 Bull SAS
COPYRIGHT: 2020 Bull SAS
*****************************************************/

#ifndef IOC_CONFIG_HPP
#define IOC_CONFIG_HPP

/****************************************************/
//std c++
#include <string>
#include <vector>

/****************************************************/
namespace IOC
{

/****************************************************/
/**
 * Object to track the configuration of the server.
**/
class Config
{
	public:
		Config(void);
		void parseArgs(int argc, const char ** argv);
		void initForUnitTests(void);
	public:
		/** IP to listen on **/
		std::string listenIP;
		/** If nvdimm is enabled, list of directories to create files in. **/
		std::vector<std::string> nvdimmMountPath;
		/** Mero ressource file. **/
		std::string meroRcFile;
		/** Enable or disable consistency check by tracking the mappgins of clients. **/
		bool consistencyCheck;
		/** Enable or disable client authentication tracking **/
		bool clientAuth;
		/** Use active polling or passive polling. **/
		bool activePolling;
};

}

#endif //IOC_CONFIG_HPP
