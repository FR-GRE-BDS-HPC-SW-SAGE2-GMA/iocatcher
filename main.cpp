//std
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <ctime>
#include <cassert>
#include <thread>
#include <string>
#include <unistd.h>
#include <argp.h>
//zero mq
//#include <zmq.hpp>
//libfabric
#include <rdma/fabric.h>
#include <rdma/fi_errno.h>
#include <rdma/fi_endpoint.h>
#include <rdma/fi_cm.h>
#include <rdma/fi_rma.h>
#include <rdma/fi_domain.h>

//g++ main.cpp -L$PWD/testinstall/lib -lfabric -I$HOME/testinstall/include -g -Wall -lzmq -lpthread

//implemented following : https://www.slideshare.net/dgoodell/ofi-libfabric-tutorial

const char *argp_program_version = "0.0.1";
const char *argp_program_bug_address = "<sebastien.valat@atos.net>";

const int MAX_THREADS = 64;
const int MAX_CLIENTS = 64;

enum RunningMode
{
	MODE_SERVER,
	MODE_CLIENT,
};

struct Options
{
	unsigned long messages;
	RunningMode mode;
	std::string ip;
	std::string port;
	int threads;
	int clients;
	bool wait;
};

enum MessageType
{
	MSG_CONN_INIT,
	MSG_ASSIGN_ID,
	MSG_PING_PONG,
};

struct Message
{
	MessageType type;
	char addr[32];
	int id;
};

struct Connection
{
	//transmission completion queue
	struct fid_cq *tx_cq;
	//revive completion queue
	struct fid_cq *rx_cq;
	//address vector
	struct fid_av *av;
	//endpoint
	struct fid_ep *ep;
	//remote addrs
	fi_addr_t remoteLiAddr[MAX_CLIENTS];
	//server address
	fi_addr_t serverLiAddr;
	//buffers
	Message msgPings[MAX_CLIENTS];
	Message msgPongs[MAX_CLIENTS];
};

#define LIBFABRIC_CHECK_STATUS(call, status) \
	do { \
		if(status != 0) { \
			fprintf(stderr, "main.cpp:%d Libfabric get failure on command %s with status %d : %s",__LINE__,call,status,fi_strerror(-status)); \
			exit(1);\
		} \
	} while(0)

void options_set_default(Options * options)
{
	//check
	assert(options != NULL);

	//default
	options->messages = 100000;
	options->mode = MODE_SERVER;
	options->ip = "127.0.0.1";
	options->port = "8556";
	options->clients = 1;
	options->threads = 1;
	options->wait = false;
}

void options_display(Options * options)
{
	//check
	assert(options != NULL);

	//infos
	printf("================ OPTIONS =================\n");
	printf("messages:     %lu\n", options->messages);
	printf("mode:         %s\n", ((options->mode == MODE_SERVER) ? "SERVER": "CLIENT"));
	printf("ip:           %s\n", options->ip.c_str());
	printf("port:         %s\n", options->port.c_str());
	printf("clients:      %d\n", options->clients);
	printf("threads:      %d\n", options->threads);
	printf("wait:         %s\n", ((options->wait)?"true":"false"));
	printf("==========================================\n");
}

error_t option_parse_opt(int key, char *arg, struct argp_state *state) {
	struct Options *options = static_cast<Options*>(state->input);
	switch (key) {
		case 'm': options->messages = atol(arg); break;
		case 's': options->mode = MODE_SERVER; break;
		case 'c': options->mode = MODE_CLIENT; break;
		case 'C': options->clients = atoi(arg); assert(options->clients <= MAX_CLIENTS); break;
		case 't': options->threads = atoi(arg); assert(options->threads <= MAX_THREADS); break;
		case 'p': options->port = arg; break;
		case 'w': options->wait = true; break;
		case ARGP_KEY_ARG: options->ip = arg; break;
		default: return ARGP_ERR_UNKNOWN;
	}
	return 0;
}

int options_parse(Options * options, int argc, char ** argv)
{
	//check
	assert(options != NULL);
	assert(argc > 0);
	assert(argv != NULL);

	//infos
	static char doc[] = "Simple libfabric ping pong";
	static char args_doc[] = "[-c/-s] [-m MESSAGES] IP...";
	static struct argp_option opts[] = { 
		{ "messages", 'm', "MESSAGES", 0, "Number of message to transmit per client."},
		{ "server", 's', 0, 0, "Set the process in server mode."},
		{ "client", 'c', 0, 0, "Set the process in client mode."},
		{ "clients", 'C', "CLIENTS", 0, "Set number of clients expected by the server."},
		{ "threads", 't', "THREADS", 0, "Set the number of threads to run in this client."},
		{ "port", 'p', "PORT", 0, "Set connection port."},
		{ "wait", 'w', 0, 0, "Use waiting mode instead of active pooling."},
		{ 0 } 
	};

	//default
	options_set_default(options);

	//parse
	static struct argp argp = { opts, option_parse_opt, args_doc, doc, 0, 0, 0 };
	int status = argp_parse(&argp, argc, argv, 0, 0, options);
	return status;
}

int wait_for_comp(struct fid_cq * cq, void ** context, bool wait)
{
	struct fi_cq_entry entry;
	int ret;

	while(1)
	{
		if (wait)
			ret = fi_cq_sread(cq, &entry, 1, NULL, -1);
		else
			ret = fi_cq_read(cq, &entry, 1);
		if (ret > 0) {
			//fprintf(stderr,"cq_read = %d\n", ret);
			if (context != NULL)
				*context = entry.op_context;
			return 0;
		} else if (ret != -FI_EAGAIN) {
			struct fi_cq_err_entry err_entry;
			fi_cq_readerr(cq, &err_entry, 0);
			fprintf(stderr, "%s %s\n", 
				fi_strerror(err_entry.err), 
				fi_cq_strerror(cq, err_entry.prov_errno, err_entry.err_data, NULL, 0));
			return ret;
		}
	}
}

void connection_setup_endpoint(Connection * connection, const Options & options, struct fi_info *fi, struct fid_domain *domain)
{
	//check
	assert(connection != NULL);
	assert(fi != NULL);
	assert(domain != NULL);

	//vars
	int err;

	//to init completion queues
	struct fi_cq_attr cq_attr;
	struct fi_av_attr av_attr;
	memset(&cq_attr, 0, sizeof(cq_attr));
	memset(&av_attr, 0, sizeof(av_attr));
	
	//setup attr
	cq_attr.format = FI_CQ_FORMAT_CONTEXT;
	cq_attr.size = fi->tx_attr->size;
	if (options.wait)
		cq_attr.wait_obj = FI_WAIT_UNSPEC;
	else
		cq_attr.wait_obj = FI_WAIT_NONE;
	printf("CQ_SIZE: %ld\n", cq_attr.size);

	//setup cq read
	err = fi_cq_open(domain, &cq_attr, &connection->tx_cq, nullptr);
	LIBFABRIC_CHECK_STATUS("fi_cq_open",err);
	
	//setup attr
	cq_attr.size = fi->rx_attr->size;

	//setup cq read
	err = fi_cq_open(domain, &cq_attr, &connection->rx_cq, nullptr);
	LIBFABRIC_CHECK_STATUS("fi_cq_open",err);

	//addres vector
	//av_attr.type = FI_AV_MAP;
	err = fi_av_open(domain, &av_attr, &connection->av, nullptr);
	LIBFABRIC_CHECK_STATUS("fi_av_open",err);

	//create endpoint
	err = fi_endpoint(domain, fi, &connection->ep, nullptr);
	LIBFABRIC_CHECK_STATUS("fi_endpoint",err);

	//bind completion queue
	err = fi_ep_bind(connection->ep, (struct fid *)connection->tx_cq, FI_SEND);
	LIBFABRIC_CHECK_STATUS("fi_ep_bind",err);
	err = fi_ep_bind(connection->ep, (struct fid *)connection->rx_cq, FI_RECV);
	LIBFABRIC_CHECK_STATUS("fi_ep_bind",err);

	//bind
	err = fi_ep_bind(connection->ep, &connection->av->fid, 0);
	LIBFABRIC_CHECK_STATUS("fi_ep_bind", err);

	//enable endpoint
	err = fi_enable(connection->ep);
	LIBFABRIC_CHECK_STATUS("fi_enable", err);
}

void handle_server_lobby(Connection & connection, const Options & options)
{
	//vars
	int err;

	//wait all client to connect
	int recievedClients = 0;
	while (recievedClients != options.clients) {
		//wait client message
		Message msgGetConn;
		err = fi_recv(connection.ep, &msgGetConn, sizeof(msgGetConn), 0, 0, NULL);
		LIBFABRIC_CHECK_STATUS("fi_recv", err);
		wait_for_comp(connection.rx_cq, NULL, options.wait);
		
		//check
		if (msgGetConn.type != MSG_CONN_INIT)
			printf("msgtype = %d\n", msgGetConn.type);
		assert(msgGetConn.type == MSG_CONN_INIT);
		printf("Get client %d !\n", recievedClients);

		//insert server address in address vector
		err = fi_av_insert(connection.av, msgGetConn.addr, 1, &connection.remoteLiAddr[recievedClients], 0, NULL);
		fprintf(stderr,"av_insert : %d\n", err);
		if (err != 1) {
			LIBFABRIC_CHECK_STATUS("fi_av_insert", -1);
		}

		//prep message to assign id
		Message msgAssignId;
		msgAssignId.type = MSG_ASSIGN_ID;
		msgAssignId.id = recievedClients;

		//send
		err = fi_send(connection.ep, &msgAssignId, sizeof(msgAssignId), 0, connection.remoteLiAddr[recievedClients], NULL);
		LIBFABRIC_CHECK_STATUS("fi_send", err);
		wait_for_comp(connection.tx_cq, NULL, options.wait);

		//inc
		recievedClients++;
	}
}

void handle_client_lobby(Connection & connection, const Options & options, struct fi_info *fi)
{
	//vars
	int err;
	size_t addrlen = sizeof(Message::addr);

	//insert server address in address vector
	err = fi_av_insert(connection.av, fi->dest_addr, 1, &connection.serverLiAddr, 0, NULL);
	if (err != 1)
		LIBFABRIC_CHECK_STATUS("fi_av_insert", -1);

	//get local addr
	Message msgConn;
	msgConn.type = MSG_CONN_INIT;
	err = fi_getname(&connection.ep->fid, msgConn.addr, &addrlen);
	LIBFABRIC_CHECK_STATUS("fi_getname", err);
	assert(addrlen <= sizeof(msgConn.addr));

	//send
	do {
		err = fi_send(connection.ep, &msgConn, sizeof(msgConn), NULL, connection.serverLiAddr, NULL);
	} while(err == -FI_EAGAIN);
	LIBFABRIC_CHECK_STATUS("fi_send", err);

	//recive id
	Message msgGetId;
	err = fi_recv(connection.ep, &msgGetId, sizeof(msgGetId), 0, 0, NULL);
	LIBFABRIC_CHECK_STATUS("fi_recv", err);
	wait_for_comp(connection.rx_cq, NULL, options.wait);
	
	//check
	assert(msgGetId.type == MSG_ASSIGN_ID);
	printf("Get client ID %d !\n", msgGetId.id);
}

void post_recives(Connection & connection, const Options & options)
{
	int err;
	for (int c = 0 ; c < options.clients ; c++) {
		err = fi_recv(connection.ep, &connection.msgPings[c], sizeof(connection.msgPings[c]), 0, 0, (void*)(unsigned long)c);
		LIBFABRIC_CHECK_STATUS("fi_recv", err);
	}
}

void bootstrap_first_ping(Connection & connection, const Options & options)
{
	//vars
	int err;
	//send all
	Message msgPingStart[MAX_CLIENTS];
	for (int c = 0 ; c < options.clients ; c++) {
		msgPingStart[c].type = MSG_PING_PONG;
		msgPingStart[c].id = c;
		err = fi_send(connection.ep, &msgPingStart[c], sizeof(msgPingStart[c]), 0, connection.remoteLiAddr[c], (void*)(unsigned long)c);
		LIBFABRIC_CHECK_STATUS("fi_send", err);	
	}

	//wait all send
	for (int c = 0 ; c < options.clients ; c++) {
		wait_for_comp(connection.tx_cq, NULL, options.wait);
	}
}

unsigned long ping_pong_server(Connection & connection, const Options & options)
{
	int err;
	unsigned long cnt = 0;
	void * opContext;
	while (cnt < options.clients * options.messages) {
		//recive id
		wait_for_comp(connection.rx_cq, &opContext, options.wait);
		unsigned long clientId = (unsigned long)opContext;
		connection.msgPongs[clientId].type = MSG_PING_PONG;
		connection.msgPongs[clientId].id = connection.msgPings[clientId].id;
		//repost before send
		err = fi_recv(connection.ep, &connection.msgPings[clientId], sizeof(connection.msgPings[clientId]), 0, 0, (void*)clientId);
		LIBFABRIC_CHECK_STATUS("fi_recv", err);
		//send
		err = fi_send(connection.ep, &connection.msgPongs[clientId], sizeof(connection.msgPongs[clientId]), 0, connection.remoteLiAddr[connection.msgPongs[clientId].id], (void*)clientId);
		LIBFABRIC_CHECK_STATUS("fi_send", err);	
		wait_for_comp(connection.tx_cq, NULL, options.wait);
		//inc
		cnt++;
	}

	return cnt;
}

unsigned long ping_pong_client(Connection & connection, const Options & options)
{
	int err;
	unsigned long cnt = 0;
	void * opContext;
	while (cnt < options.messages) {
		//recive id
		wait_for_comp(connection.rx_cq, &opContext, options.wait);
		unsigned long clientId = (unsigned long)opContext;
		connection.msgPongs[clientId].type = MSG_PING_PONG;
		connection.msgPongs[clientId].id = connection.msgPings[clientId].id;
		//repost before send
		err = fi_recv(connection.ep, &connection.msgPings[clientId], sizeof(connection.msgPings[clientId]), 0, 0, (void*)clientId);
		LIBFABRIC_CHECK_STATUS("fi_recv", err);
		//send
		err = fi_send(connection.ep, &connection.msgPongs[clientId], sizeof(connection.msgPongs[clientId]), 0, connection.serverLiAddr, (void*)clientId);
		LIBFABRIC_CHECK_STATUS("fi_send", err);	
		wait_for_comp(connection.tx_cq, NULL, options.wait);
		//inc
		cnt++;
	}

	return cnt;
}

void handle_connection(const Options & options, struct fi_info *fi, struct fid_domain *domain)
{
	//create connection
	Connection connection;

	//setup endpoint
	connection_setup_endpoint(&connection, options, fi, domain);

	//post recv
	if (options.mode == MODE_SERVER) {
		handle_server_lobby(connection, options);
	} else {
		handle_client_lobby(connection, options, fi);
	}

	//post ping recv one per client to not wait
	post_recives(connection, options);

	//bootstrap fist ping
	if (options.mode == MODE_SERVER) {
		bootstrap_first_ping(connection, options);
	}

	//time
	struct timespec start, stop;
	clock_gettime(CLOCK_MONOTONIC, &start);

	//ping pong loops
	unsigned long cnt = 0;
	if (options.mode == MODE_SERVER) {
		cnt = ping_pong_server(connection, options);
	} else {
		cnt = ping_pong_client(connection, options);
	}

	//time
	clock_gettime(CLOCK_MONOTONIC, &stop);
	double result = (stop.tv_sec - start.tv_sec) + (stop.tv_nsec - start.tv_nsec) / 1e9;    // in microseconds
	printf("Time: %g s, rate: %g kOPS\n", result, (double)cnt / result / 1000.0);
}

int main(int argc, char ** argv)
{
	//parse options
	Options options;
	options_parse(&options, argc, argv);
	options_display(&options);

	//allocate fi
	struct fi_info *hints, *fi;
	hints = fi_allocinfo();
	if (!hints) {
		printf("fi_allocinfo failed\n");
		exit(1);
	}
	hints->caps = FI_MSG;
	//hints->caps = FI_MSG;
	//hints->mode = FI_CONTEXT | FI_RX_CQ_DATA;
	//hints->mode = FI_LOCAL_MR;
	hints->ep_attr->type = FI_EP_RDM;

	//get fi_info
	int err;
	if (options.mode == MODE_SERVER)
		err = fi_getinfo(FI_VERSION(1,11), options.ip.c_str(), options.port.c_str(), FI_SOURCE, hints, &fi);
	else
		err = fi_getinfo(FI_VERSION(1,11), options.ip.c_str(), options.port.c_str(), 0, hints, &fi);
	LIBFABRIC_CHECK_STATUS("fi_getinfo",err);

	//display
	printf("NET: %s\n",fi->fabric_attr->prov_name);
	
	//setup fabric
	struct fid_fabric *fabric;
	err = fi_fabric(fi->fabric_attr, &fabric, nullptr);
	LIBFABRIC_CHECK_STATUS("fi_fabric",err);

	//setup domain
	struct fid_domain *domain;
	err = fi_domain(fabric, fi, &domain, nullptr);
	LIBFABRIC_CHECK_STATUS("fi_domain",err);

	//do work
	if (options.mode == MODE_SERVER) {
		handle_connection(options, fi, domain);
	} else {
		std::thread threads[MAX_THREADS];
		for (int t = 0 ; t < options.threads ; t++) {
			threads[t] = std::thread([&] {
				handle_connection(options, fi, domain);
			});
		}
		for (int t = 0 ; t < options.threads ; t++)
			threads[t].join();
	}

	//free ressources
	fi_freeinfo(hints);
	fi_freeinfo(fi);

	return EXIT_SUCCESS;
}
