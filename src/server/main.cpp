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
#include "base/common/Debug.hpp"
#include "core/Config.hpp"
#include "core/Server.hpp"
#include "core/Container.hpp"
#ifndef NOMERO
	#include "backends/clovis_api.h"
	#include "backends/StorageBackendMero.hpp"
#endif

/****************************************************/
using namespace IOC;

/****************************************************/
int main(int argc, const char ** argv)
{
	//parse args
	Config config;
	config.parseArgs(argc, argv);
	
	//info
	printf("LISTEN: %s\n", config.listenIP.c_str());
	if (config.nvdimmMountPath.empty() == false) {
		for (size_t i = 0 ; i < config.nvdimmMountPath.size() ; i++)
			printf("NVDIMM ENABLED: %s\n", config.nvdimmMountPath[i].c_str());
		Object::setNvdimm(config.nvdimmMountPath);
	}

	//init mero
	StorageBackend * storageBackend = NULL;
	#ifndef NOMERO
		printf("USING MERO RESSOURCE FILE: %s\n", config.meroRcFile.c_str());
		int status = c0appz_init(0, (char*)config.meroRcFile.c_str());
		assume(status == 0, "Failed to connect to Mero !");
		storageBackend = new StorageBackendMero();
	#else
		printf("NOT USING MERO\n");
	#endif

	//run server
	Server server(&config, "8556");
	server.setStorageBackend(storageBackend);

	//setup hook
	server.setOnClientConnect([](int id) {
		printf("Get client %d\n", id);
	});

	//run
	server.startStatsThread();
	server.poll();

	//remove backend
	if (storageBackend != NULL) {
		server.setStorageBackend(NULL);
		delete storageBackend;
	}

	//close clovis
	#ifndef NOMERO
		c0appz_free();
	#endif
}
