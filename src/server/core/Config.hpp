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
class Config
{
	public:
		Config(void);
		void parseArgs(int argc, const char ** argv);
		void initForUnitTests(void);
	public:
		std::string listenIP;
		std::vector<std::string> nvdimmMountPath;
		std::string meroRcFile;
		bool consistencyCheck;
		bool clientAuth;
		bool activePolling;
};

}

#endif //IOC_CONFIG_HPP
