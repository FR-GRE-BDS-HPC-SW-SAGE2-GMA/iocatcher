//gcc -fopenmp ioc-bandwidth-threads.c -I$HOME/test-rdma/usr2/include -lioc-client -L$HOME/test-rdma/usr2/lib -Wl,-rpath,$HOME/test-rdma/usr2/lib -o ioc-bandwidth-threads

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <ioc-client.h>
#include <time.h>
#include <omp.h>

const size_t total_size = 4UL*1024UL*1024UL*1024UL;
const size_t ref_repeat = 10;

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


void make_write(ioc_client_t * client, char * buffer2, size_t size, size_t seg_size, size_t repeat)
{
	//get MPI rank
	int rank = omp_get_thread_num();
	void * buffer = malloc(seg_size);
	memset(buffer,0,seg_size);

	//do
	size_t r;
	size_t offset;
	size_t base = rank * size;
	for (r = 0 ; r < repeat ; r++)
		for (offset = 0 ; offset < size ; offset += seg_size)
                        ioc_client_obj_write(client, 10, 20, buffer, seg_size, base + offset);

	free(buffer);
}

void make_read(ioc_client_t * client, char * buffer2, size_t size, size_t seg_size, size_t repeat)
{
	//get MPI rank
	int rank = omp_get_thread_num();
	void * buffer = malloc(seg_size);
	memset(buffer,0,seg_size);

	//do
	size_t r;
	size_t offset;
	size_t base = rank * size;
	for (r = 0 ; r < repeat ; r++)
		for (offset = 0 ; offset < size ; offset += seg_size)
                        ioc_client_obj_read(client, 10, 20, buffer, seg_size, base + offset);

	free(buffer);
}

double calc_bandwidth(ioc_client_t * client, char * buffer, size_t size, size_t seg_size, size_t repeat, void(*op)(ioc_client_t * client, char * buffer, size_t size, size_t seg_size, size_t repeat))
{
	//start
	struct timespec start, stop;
	clock_gettime(CLOCK_MONOTONIC, &start);

	//call to all
	#pragma omp parallel
	op(client, buffer, size, seg_size, repeat);

	//stop
	clock_gettime(CLOCK_MONOTONIC, &stop);

	//compute time
	double t = timespec_diff(&stop, &start);

	//calc bandwidth
	double bw = (double)repeat * (double)total_size / 1024.0 / 1024.0 / 1024.0 / t;

	//ok return
	return bw;
}

int main(int argc, char ** argv)
{
	//check args
	if (argc < 2) {
		fprintf(stderr, "%s {ioc_server_ip}\n", argv[0]);
		return EXIT_FAILURE;
	}

	//get MPI infos
	int world = omp_get_max_threads();

	//connect to server
	ioc_client_t * client = ioc_client_init(argv[1], "8556");

	//cal size
	size_t size = total_size / world;

	//allocate buffer
	char * buffer = malloc(size);
	memset(buffer, 0, size);

	//to ensure object is created, make a first round trip
	calc_bandwidth(client, buffer, size, 8*1024*1024, ref_repeat, make_read);
	calc_bandwidth(client, buffer, size, 8*1024*1024, ref_repeat, make_write);

	//header
	printf("#total_size = %f GB\n", (double)total_size/1024.0/1024.0/1024.0);
	printf("#threads    = %d\n", world);
	printf("#\n");
	printf("#seg_size (bytes)    read (GB/s)    twrite(GB/s)\n");

	//loop on all size
	size_t seg_size = 16 * 1024 * 1024;
	for (seg_size ; seg_size >= 4096 ; seg_size /= 2) {
		//calc repeat
		size_t repeat = ref_repeat;
		if (seg_size > 256*1024)
			repeat *= 2;

		//measure read
		double read_bw = calc_bandwidth(client, buffer, size, seg_size, repeat, make_read);
		double write_bw = calc_bandwidth(client, buffer, size, seg_size, repeat, make_write);

		//print
		printf("%zu %f %f\n", seg_size, read_bw, write_bw);
	}

	//close connection
	ioc_client_fini(client);

	//ok
	return EXIT_SUCCESS;
}
