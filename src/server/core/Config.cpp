/*****************************************************
            PROJECT  : IO Catcher
            LICENSE  : Apache 2.0
			COPYRIGHT: 2020 Bull SAS
COPYRIGHT: 2020 Bull SAS
*****************************************************/

/****************************************************/
//std
#include <cstdlib>
//linux
#include <argp.h>
//internal
#include "Config.hpp"

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
	{ "active-polling", 'p', 0, 0, "Enable active polling."},
	{ "no-auth", 'a', 0, 0, "Disable client auth."},
	{ 0 } 
};

/****************************************************/
static std::vector<std::string> splitToVector(const std::string & value)
{
	//vars
	std::vector<std::string> output;
	std::string::size_type prev_pos = 0, pos = 0;

	//loop on separators
	while((pos = value.find(',', pos)) != std::string::npos)
	{
		std::string substring(value.substr(prev_pos, pos-prev_pos));
		output.push_back(substring);
		prev_pos = ++pos;
	}

	//push last
	output.push_back(value.substr(prev_pos, pos-prev_pos));

	//return
	return output;
}

/****************************************************/
static error_t parseOptions(int key, char *arg, struct argp_state *state) {
	Config *config = (Config *)state->input;
	switch (key) {
		case 'n': config->nvdimmMountPath = splitToVector(arg); break;
		case 'l': config->listenIP = arg; break;
		case 'c': config->consistencyCheck = false; break;
		case 'p': config->activePolling = true; break;
		case 'a': config->clientAuth = false; break;
		case ARGP_KEY_NO_ARGS: argp_usage (state); break;
		case ARGP_KEY_ARG: 
			if (config->listenIP.empty())
				config->listenIP = arg;
			else
				argp_usage (state);
			break;
		default: return ARGP_ERR_UNKNOWN;
	}   
	return 0;
}

/****************************************************/
Config::Config(void)
{
	this->listenIP = "";
	this->meroRcFile = "mero_ressource_file.rc";
	this->consistencyCheck = true;
	this->clientAuth = true;
	this->activePolling = false;
}

/****************************************************/
void Config::initForUnitTests(void)
{
	this->listenIP = "127.0.0.1";
	this->activePolling = true;
}

/****************************************************/
void Config::parseArgs(int argc, const char ** argv)
{
	//parse args
	static struct argp argp = { options, parseOptions, args_doc, doc, 0, 0, 0 };
	argp_parse(&argp, argc, (char**)argv, 0, 0, this);

	//check
	if (this->listenIP.empty()) {
		fprintf(stderr, "Usage: iocatcher-server {IP}\n");
		exit(1);
	}
}
