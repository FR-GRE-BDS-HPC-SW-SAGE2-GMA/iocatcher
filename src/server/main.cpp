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
#include <argp.h>
//linux
#include <unistd.h>
#include <sys/mman.h>
#include <cstdlib>
#include <sys/uio.h>
//local
#include "core/Container.hpp"
#include "core/ServerActions.hpp"
#ifndef NOMERO
	#include "core/clovis_api.h"
#endif

/****************************************************/
using namespace IOC;

/****************************************************/
const char *argp_program_version = "iocatcher-server";
const char *argp_program_bug_address = "<sebastien.valat@atos.net>";
static char doc[] = "A cache between Mero and ummap-io";
static char args_doc[] = "LISTEN_IP";
static struct argp_option options[] = { 
	{ "nvdimm", 'n', "PATH", 0, "Store data in nvdimm at the given PATH."},
	{ "merofile", 'm', "PATH", 0, "Mero ressource file to use."},
	{ "no-consistency-check", 'c', 0, 0, "Disable consistency check."},
	{ 0 } 
};

/****************************************************/
struct Arguments
{
	Arguments(void);
	const char * listen;
	const char * nvdimm;
	const char * merofile;
	bool consistencyCheck;
};

/****************************************************/
Arguments::Arguments(void)
{
	this->listen = NULL;
	this->nvdimm = NULL;
	this->merofile = "mero_ressource_file.rc";
	this->consistencyCheck = true;
}

/****************************************************/
error_t parseOptions(int key, char *arg, struct argp_state *state) {
	Arguments *arguments = (Arguments *)state->input;
	switch (key) {
		case 'n': arguments->nvdimm = arg; break;
		case 'l': arguments->listen = arg; break;
		case 'c': arguments->consistencyCheck = false; break;
		case ARGP_KEY_NO_ARGS: argp_usage (state); break;
		case ARGP_KEY_ARG: 
			if (arguments->listen == NULL)
				arguments->listen = arg;
			else
				argp_usage (state);
			break;
		default: return ARGP_ERR_UNKNOWN;
	}   
	return 0;
}

/****************************************************/
int main(int argc, char ** argv)
{
	//parse args
	Arguments arguments;
	static struct argp argp = { options, parseOptions, args_doc, doc, 0, 0, 0 };
	argp_parse(&argp, argc, argv, 0, 0, &arguments);

	//check
	if (arguments.listen == NULL) {
		fprintf(stderr, "Usage: iocatcher-server {IP}\n");
		return 1;
	} else {
		printf("LISTEN: %s\n", arguments.listen);
	}

	//nvdimm
	if (arguments.nvdimm != NULL) {
		printf("NVDIMM ENABLED: %s\n", arguments.nvdimm);
		Object::setNvdimm(arguments.nvdimm);
	}

	//init mero
	#ifndef NOMERO
		printf("USING MERO RESSOURCE FILE: %s\n", arguments.merofile);
		c0appz_init(0, (char*)arguments.merofile);
	#else
		printf("NOT USING MERO\n");
	#endif

	//dispatch
	gblConsistencyCheck = arguments.consistencyCheck;

	//stats thread
	bool run = true;
	std::thread statThread = std::thread([&run]{
		while (run) {
			sleep(1);
			printf("Read: %g GB/s, Write: %g GB/s\n", (double)gblReadSize/1.0/1024.0/1024.0/1024.0, (double) gblWriteSize/1.0/1024.0/1024.0/1024.0);
			gblReadSize = 0;
			gblWriteSize = 0;
		}
	});

	//init domain & conn
	LibfabricDomain domain(arguments.listen, "8556", true);
	domain.setMsgBuffeSize(sizeof(LibfabricMessage)+(IOC_EAGER_MAX_READ));
	LibfabricConnection connection(&domain, false);
	connection.postRecives(1024*1024, 64);

	//test readonly registration
	/*printf("test\n");
	void * ptr = mmap(NULL, 16*1024*1024, PROT_READ, MAP_ANON | MAP_PRIVATE, 0, 0);
	domain.registerSegment(ptr, 16*1024*1024, true, false, false);
	domain.unregisterSegment(ptr, 16*1024*1024);
	munmap(ptr, 16*1024*1024);
	printf("ok\n");*/

	// client lobby
	connection.setHooks([](int id) {
		printf("Get client %d\n", id);
	});
	
	//register read hooks
	Container container(&domain, 8*1024*1024);

	//register hooks
	setupPingPong(connection);
	setupObjRead(connection, container);
	setupObjWrite(connection, container);
	setupObjFlush(connection, container);
	setupObjCreate(connection, container);
	setupObjRangeRegister(connection, container);
	setupObjUnregisterRange(connection, container);

	// poll
	for(;;) {
		connection.poll(false);
	}

	//status thread
	run = false;
	statThread.join();

	//close clovis
	#ifndef NOMERO
		c0appz_free();
	#endif
}
