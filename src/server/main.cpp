/*****************************************************
			 PROJECT  : IO Catcher
			 LICENSE  : Apache 2.0
			COPYRIGHT: 2020 Bull SAS
			 COPYRIGHT: 2020 Bull SAS
*****************************************************/

/****************************************************/
//std
#include <cstdio>
#include <cassert>
#include <cstring>
#include <thread>
//linux
#include <argp.h>
#include <unistd.h>
#include <sys/mman.h>
#include <cstdlib>
#include <sys/uio.h>
//local
#include "core/Config.hpp"
#include "core/Server.hpp"
#include "core/Container.hpp"
#ifndef NOMERO
	#include "core/clovis_api.h"
#endif

/****************************************************/
using namespace IOC;

/****************************************************/
int main(int argc, char ** argv)
{
	//parse args
	Config config;
	config.parseArgs(argc, argv);
	
	//info
	printf("LISTEN: %s\n", config.listenIP.c_str());
	if (config.nvdimmMountPath.empty())
		printf("NVDIMM ENABLED: %s\n", config.nvdimmMountPath);

	//init mero
	#ifndef NOMERO
		printf("USING MERO RESSOURCE FILE: %s\n", config.meroRcFile);
		c0appz_init(0, (char*)config.meroRcFile);
	#else
		printf("NOT USING MERO\n");
	#endif

	//run server
	Server server(&config, "8556");

	//setup hook
	server.setOnClientConnect([](int id) {
		printf("Get client %d\n", id);
	});

	//run
	server.startStatsThread();
	server.poll();

	//close clovis
	#ifndef NOMERO
		c0appz_free();
	#endif
}
