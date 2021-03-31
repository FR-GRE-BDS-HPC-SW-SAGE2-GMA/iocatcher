#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <ummap/ummap.h>
#include <mpi.h>

#define SIZE (2UL*1024UL*1024UL*1024UL)
#define SEGSIZE (2UL*1024UL*1024UL)
#define ITERATIONS 4

typedef enum {
	MODE_COPY,
	MODE_SWITCH,
	MODE_COW,
} cow_mode_t;

const char * mode_names[] = {
	"Ummap + copy",
	"Ummap + switch",
	"Ummap + cow",
};

typedef struct {
	size_t size;
	size_t segsize;
	size_t iterations;
	size_t offset;
	size_t comp_size;
	const char * driver_uri;
	const char * policy_uri;
	int rank;
} config_t;

static inline double timespec_diff(struct timespec *a, struct timespec *b) {
	struct timespec result;
	result.tv_sec  = a->tv_sec  - b->tv_sec;
	result.tv_nsec = a->tv_nsec - b->tv_nsec;
	if (result.tv_nsec < 0) {
		--result.tv_sec;
		result.tv_nsec += 1000000000L;
	}
	return (double)result.tv_sec + (double)result.tv_nsec / (double)1e9;
}


void init(const config_t * config, char * ptr)
{
	size_t i;
	for (i = 0 ; i < config->size ; i+=4096)
		ptr[i] = 1;
}

void compute(const config_t * config, char * ptr)
{
	size_t i;
	for (i = 0 ; i < config->comp_size ; i+=4096)
		ptr[i] += i;
}

void copy(const config_t * config, char * dest, const char * src)
{
	size_t i;
	#pragma omp parallel for
	for (i = 0 ; i < config->size; i++)
		dest[i] = src[i];
}

void run_copy(const config_t * config)
{
	//vars
	size_t it;

	//establish the mapping
	ummap_uri_set_variable_int("step", 0);
	ummap_driver_t * driver = ummap_driver_create_uri(config->driver_uri);
	ummap_policy_t * policy = ummap_policy_create_uri(config->policy_uri, true);
	char * ptr = ummap(NULL, config->size, config->segsize, 0, PROT_READ|PROT_WRITE, UMMAP_NO_FIRST_READ, driver, policy, NULL);

	//init content
	init(config, ptr);

	//compute
	for (it = 0 ; it < config->iterations ; it++) {
		//compute
		compute(config, ptr);

		//flush
		MPI_Barrier(MPI_COMM_WORLD);
		umsync(ptr, 0, false);

		//create a new mapping
		ummap_uri_set_variable_int("step", it % 2);
		driver = ummap_driver_create_uri(config->driver_uri);
		policy = ummap_policy_create_uri(config->policy_uri, true);
		char * tmp = ummap(NULL, config->size, config->segsize, 0, PROT_READ|PROT_WRITE, UMMAP_NO_FIRST_READ, driver, policy, NULL);

		//copy
		copy(config, tmp, ptr);

		//unmap old
		umunmap(ptr, false);

		//swap
		ptr = tmp;
	}

	//unmap
	umunmap(ptr, false);
}

void run_switch(const config_t * config)
{
	//vars
	size_t it;

	//establish the mapping
	ummap_uri_set_variable_int("step", 0);
	ummap_driver_t * driver = ummap_driver_create_uri(config->driver_uri);
	ummap_policy_t * policy = ummap_policy_create_uri(config->policy_uri, true);
	char * ptr = ummap(NULL, config->size, config->segsize, 0, PROT_READ|PROT_WRITE, UMMAP_NO_FIRST_READ, driver, policy, NULL);

	//init content
	init(config, ptr);

	//compute
	for (it = 0 ; it < config->iterations ; it++) {
		//compute
		compute(config, ptr);

		//flush
		MPI_Barrier(MPI_COMM_WORLD);
		umsync(ptr, 0, false);

		//create a new mapping
		ummap_uri_set_variable_int("step", it % 2);
		ummap_switch_uri(ptr, config->driver_uri, UMMAP_MARK_CLEAN_DIRTY);
	}

	//unmap
	umunmap(ptr, false);
}

void run_cow(const config_t * config)
{
	//vars
	size_t it;

	//establish the mapping
	ummap_uri_set_variable_int("step", 0);
	ummap_driver_t * driver = ummap_driver_create_uri(config->driver_uri);
	ummap_policy_t * policy = ummap_policy_create_uri(config->policy_uri, true);
	char * ptr = ummap(NULL, config->size, config->segsize, 0, PROT_READ|PROT_WRITE, UMMAP_NO_FIRST_READ, driver, policy, NULL);

	//init content
	init(config, ptr);

	//compute
	for (it = 0 ; it < config->iterations ; it++) {
		//compute
		compute(config, ptr);

		//flush
		MPI_Barrier(MPI_COMM_WORLD);
		umsync(ptr, 0, false);

		//create a new mapping
		ummap_uri_set_variable_int("step", it % 2);
		ummap_cow_uri(ptr, config->driver_uri, true);
	}

	//unmap
	umunmap(ptr, false);
}

void run(const config_t * config, cow_mode_t mode)
{
	switch(mode) {
		case MODE_COPY:
			run_copy(config);
			break;
		case MODE_SWITCH:
			run_switch(config);
			break;
		case MODE_COW:
			run_cow(config);
			break;
	};
}

void measure(const config_t * config, cow_mode_t mode)
{
	MPI_Barrier(MPI_COMM_WORLD);

	//start
	struct timespec start, stop;
	clock_gettime(CLOCK_MONOTONIC, &start);

	//run
	run(config, mode);
	MPI_Barrier(MPI_COMM_WORLD);

	//stop
	clock_gettime(CLOCK_MONOTONIC, &stop);

	//display
	if (config->rank == 0) 
		printf("%-20s : %0.03f seconds\n", mode_names[mode], timespec_diff(&stop, &start));
}

int main(int argc, char ** argv)
{
	//check
	if (argc != 2) {
		fprintf(stderr,"Usage %s {ioc_server_ip}\n", argv[0]);
		return EXIT_FAILURE;
	}

	//init
	MPI_Init(&argc, &argv);
	int rank, world;
	MPI_Comm_size(MPI_COMM_WORLD, &world);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	//extract
	const char * ioc_server = argv[1];

	//config
	config_t config;
	config.size = SIZE;
	config.segsize = SEGSIZE;
	config.iterations = ITERATIONS;
	config.offset = 0;
	config.comp_size = SIZE;
	config.driver_uri = "ioc://1{rank}:2{step}";
	config.policy_uri = "none://";
	config.rank = rank;

	//print config
	if (rank == 0) {
		printf("==================== Config =====================\n");
		printf("Size          : %0.2f GB\n", (float)config.size/1024.0/1024.0/1024.0);
		printf("Segment size  : %0.2f MB\n", (float)config.segsize/1024.0/1024.0/1024.0);
		printf("Iterations    : %lu\n", config.iterations);
		printf("Driver        : %s\n", config.driver_uri);
		//printf("policy        : %s\n", config.policy_uri);
		printf("Ranks         : %d\n", world);
	}

	//init
	ummap_init();
	ummap_config_ioc_init_options(ioc_server, "8556");
	ummap_uri_set_variable_int("rank", rank);

	//make runs
	if (rank == 0) printf("\n================= Compute on all ================\n");
	measure(&config, MODE_COPY);
	measure(&config, MODE_SWITCH);
	measure(&config, MODE_COW);

	//modify only a part
	if (rank == 0) printf("\n================= Compute on 1/4 ================\n");
	config.comp_size /= 4;
	measure(&config, MODE_COPY);
	measure(&config, MODE_SWITCH);
	measure(&config, MODE_COW);

	//fini
	ummap_finalize();
	MPI_Finalize();

	//Ok
	return EXIT_SUCCESS;
}
