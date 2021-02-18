#undef NDEBUG
//std
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <ctime>
#include <cassert>
#include <thread>
#include <string>
#include <cstring>
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

/** Setup for argp module to generate help message. **/
const char *argp_program_version = "0.0.1";
/** Setup for argp module to generate help message. **/
const char *argp_program_bug_address = "<sebastien.valat@atos.net>";

/** Maximum number of threads allowed. **/
const int MAX_THREADS = 64;
/** Maximum number of clients allowed. **/
const int MAX_CLIENTS = 64;

/** Define the role, client or server. **/
enum Role
{
	ROLE_SERVER,
	ROLE_CLIENT,
};

/** Option struct to configure the application. **/
struct Options
{
	/** Number of message to send on client side, this will represent.
	 * (client * message) on the server side. 
	**/
	unsigned long messages;
	/** Role to apply in the current process. **/
	Role role;
	/** Address IP to listen or connect on. **/
	std::string ip;
	/** Port to listen or connect on. **/
	std::string port;
	/** Numuber of client threads to launch. **/
	int threads;
	/** Number of client to wait for on the server side. **/
	int clients;
	/** Use passive poling instead or active poling. **/
	bool wait;
	/** Size of the messages to exchange. **/
	size_t msgSize;
};

/** Define the type of message exchanged. **/
enum MessageType
{
	/** Message use after connection to send the client address to the server. **/
	MSG_CONN_INIT,
	/** Answer to MSG_CONN_INIT to assign an ID to the client. **/
	MSG_ASSIGN_ID,
	/** This is a ping pong message to respond to. **/
	MSG_PING_PONG,
};

/** Message header to be exchaned for the ping pong. **/
struct Message
{
	/** Type of message **/
	MessageType type;
	/** Client to address to be send to the server on MSG_CONN_INIT **/
	char addr[32];
	/** Client ID so the server know to who to respond. **/
	int id;
};

/**
 * Keep all informations related to a connection to be used by communications
 * functions.
**/
struct Connection
{
	/** Transmission completion queue. **/
	struct fid_cq *tx_cq;
	//** Revive completion queue. **/
	struct fid_cq *rx_cq;
	/** Address vector. **/
	struct fid_av *av;
	/** Endpoint **/
	struct fid_ep *ep;
	/** Remote addresses. **/
	fi_addr_t remoteLiAddr[MAX_CLIENTS];
	/** Server address **/
	fi_addr_t serverLiAddr;
	/** Buffer for reception. **/
	void * bufferPings[MAX_CLIENTS];
	/** Buffer for emission. **/
	void * bufferPongs[MAX_CLIENTS];
};

/**
 * Macro to check libfabric function status and stop with an error message
 * in case of failure.
**/
#define LIBFABRIC_CHECK_STATUS(call, status) \
	do { \
		if(status != 0) { \
			fprintf(stderr, "main.cpp:%d Libfabric get failure on command %s with status %d : %s",__LINE__,call,status,fi_strerror(-status)); \
			exit(1);\
		} \
	} while(0)

/**
 * Set the default options.
 * @param options Structure to fill.
**/
void options_set_default(Options * options)
{
	//check
	assert(options != NULL);

	//default
	options->messages = 100000;
	options->role = ROLE_SERVER;
	options->ip = "127.0.0.1";
	options->port = "8556";
	options->clients = 1;
	options->threads = 1;
	options->wait = false;
	options->msgSize = sizeof(Message);
}

/**
 * Display the setup to be used.
 * @param options Structure containing the parameters to display.
**/
void options_display(Options * options)
{
	//check
	assert(options != NULL);

	//infos
	printf("================ OPTIONS =================\n");
	printf("messages:     %lu\n", options->messages);
	printf("role:         %s\n", ((options->role == ROLE_SERVER) ? "SERVER": "CLIENT"));
	printf("ip:           %s\n", options->ip.c_str());
	printf("port:         %s\n", options->port.c_str());
	printf("clients:      %d\n", options->clients);
	printf("threads:      %d\n", options->threads);
	printf("wait:         %s\n", ((options->wait)?"true":"false"));
	printf("size:         %zu\n", options->msgSize);
	printf("==========================================\n");
}

/**
 * Handler to be called bu the parser to apply options on the option state.
 * @param key Key defining the current option to be parsed.
 * @param arg Define the argument value.
 * @param state To access to the option structure.
 * @return Return 0 on success and another value in case of error.
**/
error_t option_parse_opt(int key, char *arg, struct argp_state *state) {
	struct Options *options = static_cast<Options*>(state->input);
	switch (key) {
		case 'm': options->messages = atol(arg); break;
		case 's': options->role = ROLE_SERVER; break;
		case 'c': options->role = ROLE_CLIENT; break;
		case 'C': options->clients = atoi(arg); assert(options->clients <= MAX_CLIENTS); break;
		case 't': options->threads = atoi(arg); assert(options->threads <= MAX_THREADS); break;
		case 'p': options->port = arg; break;
		case 'w': options->wait = true; break;
		case 'S': options->msgSize = atol(arg); assert(options->msgSize >= sizeof(Message)); break;
		case ARGP_KEY_ARG: options->ip = arg; break;
		default: return ARGP_ERR_UNKNOWN;
	}
	return 0;
}

/**
 * Setup the argp parser and call it to parse the program options and setup
 * the option struct.
 * @param options Option struct to be filled by the parser.
 * @param argc Number of parameters recieved by the program.
 * @param argv List of parameters recived by the program.
 * @return Return 0 on success, another value on error.
**/
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
		{ "size", 'S', "SIZE", 0, "Message size for ping pong."},
		{ 0 } 
	};

	//default
	options_set_default(options);

	//parse
	static struct argp argp = { opts, option_parse_opt, args_doc, doc, 0, 0, 0 };
	int status = argp_parse(&argp, argc, argv, 0, 0, options);
	return status;
}

/**
 * Wait on the given completion queue.
 * @param cq Define the completion queue to wait on.
 * @param context Define the context object to be recived to identify which
 * operation has finished.
 * @param wait Define if we should use the active or passive polling.
 * @return Return the number of completion recived. Should be 1 with the current
 * implementation.
**/
int wait_for_comp(struct fid_cq * cq, void ** context, bool wait)
{
	struct fi_cq_msg_entry entry;
	int ret;

	while(1)
	{
		if (wait)
			ret = fi_cq_sread(cq, &entry, 1, NULL, -1);
		else
			ret = fi_cq_read(cq, &entry, 1);
		if (ret > 0) {
			//fprintf(stderr,"cq_read = %d; size = %zu\n", ret, entry.len);
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

/**
 * Use the given fi & domain structure to establish a connection arround an 
 * endpoint.
 * @param connection Pointer to the structure to fill.
 * @param options Define the options used to configure the connection.
 * @param fi Define the libfabric service configuration to be used.
 * @param domaine Define the libfabric domain to attach the endpoint to.
**/
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
	//cq_attr.format = FI_CQ_FORMAT_CONTEXT;
	cq_attr.format = FI_CQ_FORMAT_MSG;
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

/**
 * To be used by server to wait all the clients to connect and be assigned
 * to a client ID. This function exit only when all the clients are therer.
 * This function should recived only MSG_CONN_INIT messages.
 * @param connection Define the connection to read on.
 * @param options Define the options to know how many clients to wait.
**/
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
		printf("av_insert : %d\n", err);
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

/**
 * On the client side establish connection by completing the address vector,
 * send the CONN_INI message and wait for client ID assignement.
 * @param connection Define the connection to read on.
 * @param options Define the options to know how many clients to wait.
**/
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

/**
 * Post recives before starting to send messages so we are ready.
 * @param connection Define the connection to read on.
 * @param options Define the options to know how many clients to read from.
**/
void post_recives(Connection & connection, const Options & options)
{
	int err;
	for (int c = 0 ; c < options.clients ; c++) {
		err = fi_recv(connection.ep, connection.bufferPings[c], options.msgSize, 0, 0, (void*)(unsigned long)c);
		LIBFABRIC_CHECK_STATUS("fi_recv", err);
	}
}

/**
 * On the server side, when we are ready, send the first ping to all clients.
 * @param connection Define the connection to use for sending.
 * @param options Define the options to know how many clients to contact.
**/
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

/**
 * On the server side, wait for reception of ping request and send back a pong.
 * @param connection Define the connection to use.
 * @param options Define the options to know how many clients to wait.
**/
unsigned long ping_pong_server(Connection & connection, const Options & options)
{
	int err;
	unsigned long cnt = 0;
	void * opContext;
	while (cnt < options.clients * options.messages) {
		//recive id
		wait_for_comp(connection.rx_cq, &opContext, options.wait);
		unsigned long clientId = (unsigned long)opContext;
		Message * msgPings = (Message*)connection.bufferPings[clientId];
		Message * msgPongs = (Message*)connection.bufferPongs[clientId];
		msgPongs->type = MSG_PING_PONG;
		msgPongs->id = msgPings->id;

		/*void * buf = malloc(options.msgSize);
		memcpy(buf, connection.bufferPings[clientId], options.msgSize);
		free(buf);*/

		//repost before send
		err = fi_recv(connection.ep, connection.bufferPings[clientId], options.msgSize, 0, 0, (void*)clientId);
		LIBFABRIC_CHECK_STATUS("fi_recv", err);
		//send
		err = fi_send(connection.ep, connection.bufferPongs[clientId], options.msgSize, 0, connection.remoteLiAddr[msgPongs->id], (void*)clientId);
		LIBFABRIC_CHECK_STATUS("fi_send", err);	
		wait_for_comp(connection.tx_cq, NULL, options.wait);
		//inc
		cnt++;
	}

	return cnt;
}

/**
 * On the server side, wait for reception of pong request and send back a ping.
 * @param connection Define the connection to use.
 * @param options Define the options to know how many clients to wait.
**/
unsigned long ping_pong_client(Connection & connection, const Options & options)
{
	int err;
	unsigned long cnt = 0;
	void * opContext;
	while (cnt < options.messages) {
		//recive id
		wait_for_comp(connection.rx_cq, &opContext, options.wait);
		unsigned long clientId = (unsigned long)opContext;
		Message * msgPings = (Message*)connection.bufferPings[clientId];
		Message * msgPongs = (Message*)connection.bufferPongs[clientId];
		msgPongs->type = MSG_PING_PONG;
		msgPongs->id = msgPings->id;
		//repost before send
		err = fi_recv(connection.ep, connection.bufferPings[clientId], options.msgSize, 0, 0, (void*)clientId);
		LIBFABRIC_CHECK_STATUS("fi_recv", err);
		//send
		err = fi_send(connection.ep, connection.bufferPongs[clientId], options.msgSize, 0, connection.serverLiAddr, (void*)clientId);
		LIBFABRIC_CHECK_STATUS("fi_send", err);	
		wait_for_comp(connection.tx_cq, NULL, options.wait);
		//inc
		cnt++;
	}

	return cnt;
}

/**
 * Apply operation no the given connection. It setup the endpoint, connect,
 * handle connection messages and finish by making the ping pong communications.
 * It finished by printing the message rate and bandwidth.
 * @param options Options to use to configuration the actions.
 * @param fi Define the fabric to be used.
 * @param domain Define the domain to which attach the connections and register
 * the memory regions.
**/
void handle_connection(const Options & options, struct fi_info *fi, struct fid_domain *domain)
{
	//create connection
	Connection connection;

	//allocate buffers
	for (int i = 0 ; i < options.clients ; i++) {
		connection.bufferPings[i] = malloc(options.msgSize);
		connection.bufferPongs[i] = malloc(options.msgSize);
	}

	//setup endpoint
	connection_setup_endpoint(&connection, options, fi, domain);

	//mr register
	for (int i = 0 ; i < options.clients ; i++) {
		fid_mr * mr;
		fi_mr_reg(domain, connection.bufferPings[i], options.msgSize, FI_RECV, 0, 0, 0, &mr, NULL);
		fi_mr_reg(domain, connection.bufferPongs[i], options.msgSize, FI_SEND, 0, 0, 0, &mr, NULL);
	}

	//post recv
	if (options.role == ROLE_SERVER) {
		handle_server_lobby(connection, options);
	} else {
		handle_client_lobby(connection, options, fi);
	}

	//post ping recv one per client to not wait
	post_recives(connection, options);

	//bootstrap fist ping
	if (options.role == ROLE_SERVER) {
		bootstrap_first_ping(connection, options);
	}

	//time
	struct timespec start, stop;
	clock_gettime(CLOCK_MONOTONIC, &start);

	//ping pong loops
	unsigned long cnt = 0;
	if (options.role == ROLE_SERVER) {
		cnt = ping_pong_server(connection, options);
	} else {
		cnt = ping_pong_client(connection, options);
		wait_for_comp(connection.rx_cq, NULL, options.wait);
	}

	//time
	clock_gettime(CLOCK_MONOTONIC, &stop);
	double result = (stop.tv_sec - start.tv_sec) + (stop.tv_nsec - start.tv_nsec) / 1e9;    // in microseconds
	double rate = (double)cnt / result / 1000.0;
	double bandwidth = 8.0 * (double)cnt * (double)options.msgSize / result / 1000.0 / 1000.0 / 1000.0;
	double bandwidth2 = (double)cnt * (double)options.msgSize / result / 1024.0 / 1024.0 / 1024.0;
	printf("Time: %g s, rate: %g kOPS, bandwidth: %g GBits/s, %g Gbytes/s, size: %zu\n", result, rate, bandwidth, bandwidth2, options.msgSize);

	//free buffers
	for (int i = 0 ; i < options.clients ; i++) {
		free(connection.bufferPings[i]);
		free(connection.bufferPongs[i]);
	}
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
	if (options.role == ROLE_SERVER)
		err = fi_getinfo(FI_VERSION(1,6), options.ip.c_str(), options.port.c_str(), FI_SOURCE, hints, &fi);
	else
		err = fi_getinfo(FI_VERSION(1,6), options.ip.c_str(), options.port.c_str(), 0, hints, &fi);
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
	if (options.role == ROLE_SERVER) {
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
