//mpicc ioc-ummap-bandwidth-mpi.c -I$HOME/test-rdma/usr2/include -lioc-client -lummap-io -L$HOME/test-rdma/usr2/lib -Wl,-rpath,$HOME/test-rdma/usr2/lib -o ioc-ummap-bandwidth-mpi

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <ioc-client.h>
#include <time.h>
#include <mpi.h>
#include <ummap/ummap.h>
#include <omp.h>

const size_t total_size = 650UL*1024UL*1024UL*1024UL;
const size_t ref_repeat = 1;

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

void make_ummap_read(ioc_client_t * client, char * buffer0, size_t size, size_t seg_size, size_t repeat)
{
	//get MPI rank
	int rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	size_t threads = omp_get_max_threads();

	//calc base
	size_t base = rank * size;

	//ummap
	ummap_driver_t * driver = ummap_driver_create_ioc(client, 10, 20, rank == 0);
	ummap_policy_t * policy = ummap_policy_create_fifo(2 * threads * seg_size, true);
	int flags = 0;
	if (seg_size <= 131072)
		flags |= UMMAP_THREAD_UNSAFE;
	char * buffer = ummap(NULL, size, seg_size, base, PROT_READ|PROT_WRITE, flags, driver, policy, NULL);
	//access
	size_t r;
	size_t offset;
	size_t sum = 0;
	for (r = 0 ; r < repeat ; r++) {
		#pragma omp parallel for
		for (offset = 0 ; offset < size ; offset +=seg_size)
			sum+=buffer[offset];
	}

	//unmap
	umunmap(buffer, false);
}

void make_ummap_write(ioc_client_t * client, char * buffer0, size_t size, size_t seg_size, size_t repeat)
{
	//get MPI rank
	int rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	size_t threads = omp_get_max_threads();

	//calc base
	size_t base = rank * size;

	//ummap
	ummap_driver_t * driver = ummap_driver_create_ioc(client, 10, 20, rank == 0);
	ummap_policy_t * policy = ummap_policy_create_fifo(2 * threads * seg_size, true);
	int flags = 0;
	if (seg_size <= 131072)
		flags |= UMMAP_THREAD_UNSAFE;
	char * buffer = ummap(NULL, size, seg_size, base, PROT_READ|PROT_WRITE, UMMAP_NO_FIRST_READ|flags, driver, policy, NULL);

	//access
	size_t r;
	size_t offset;
	for (r = 0 ; r < repeat ; r++) {
		ummap_skip_first_read(buffer);
		#pragma omp parallel for
		for (offset = 0 ; offset < size ; offset += seg_size)
			buffer[offset]++;
	}

	//unmap
	umunmap(buffer, false);
}

void make_write(ioc_client_t * client, char * buffer, size_t size, size_t seg_size, size_t repeat)
{
	//get MPI rank
	int rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	//do
	size_t r;
	size_t offset;
	size_t base = rank * size;
	for (r = 0 ; r < repeat ; r++)
		for (offset = 0 ; offset < size ; offset += seg_size)
                        ioc_client_obj_write(client, 10, 20, buffer, seg_size, base + offset);

}

void make_read(ioc_client_t * client, char * buffer, size_t size, size_t seg_size, size_t repeat)
{
	//get MPI rank
	int rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	//do
	size_t r;
	size_t offset;
	size_t base = rank * size;
	for (r = 0 ; r < repeat ; r++)
		for (offset = 0 ; offset < size ; offset += seg_size)
                        ioc_client_obj_read(client, 10, 20, buffer, seg_size, base + offset);

}

double calc_bandwidth(ioc_client_t * client, char * buffer, size_t size, size_t seg_size, size_t repeat, void(*op)(ioc_client_t * client, char * buffer, size_t size, size_t seg_size, size_t repeat))
{
	//wait all
	MPI_Barrier(MPI_COMM_WORLD);

	//start
	struct timespec start, stop;
	clock_gettime(CLOCK_MONOTONIC, &start);

	//call to all
	op(client, buffer, size, seg_size, repeat);

	//wait all
	MPI_Barrier(MPI_COMM_WORLD);

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

	//init MPI
	MPI_Init(&argc, &argv);

	//init ummapio
	ummap_init();

	//get MPI infos
	int rank;
	int world;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &world);

	//connect to server
	ioc_client_t * client = ioc_client_init(argv[1], "8556");

	//cal size
	size_t size = total_size / world;

	//allocate buffer
	char * buffer = NULL;
	//char * buffer = malloc(size);
	//memset(buffer, 0, size);

	//to ensure object is created, make a first round trip
	//calc_bandwidth(client, buffer, size, 8*1024*1024, ref_repeat, make_read);
	//calc_bandwidth(client, buffer, size, 8*1024*1024, ref_repeat, make_write);
	size_t seg_size = 16 * 1024 * 1024;
	calc_bandwidth(client, buffer, size, seg_size, ref_repeat, make_ummap_read);
	calc_bandwidth(client, buffer, size, seg_size, ref_repeat, make_ummap_write);

	//header
	if (rank == 0) {
		printf("#total_size=%f GB\n", (double)total_size/1024.0/1024.0/1024.0);
		printf("#world_size=%d\n", world);
		printf("#seg_size (bytes)    read (GB/s)    twrite(GB/s)\n");
	}

	//loop on all size
	//size_t seg_size = 128 * 1024 * 1024;
	//for ( ; seg_size >= 4096 ; seg_size /= 2) {
	{
		//calc repeat
		size_t repeat = ref_repeat;
		//if (seg_size > 256*1024)
		//	repeat *= 2;

		//measure read
		//double read_bw = calc_bandwidth(client, buffer, size, seg_size, repeat, make_read);
		//double write_bw = calc_bandwidth(client, buffer, size, seg_size, repeat, make_write);
		double read_bw = calc_bandwidth(client, buffer, size, seg_size, repeat, make_ummap_read);
		double write_bw = calc_bandwidth(client, buffer, size, seg_size, repeat, make_ummap_write);

		//print
		if (rank == 0)
			printf("%zu %f %f\n", seg_size, read_bw, write_bw);
	}

	//close connection
	ioc_client_fini(client);

	//fini ummap
	ummap_finalize();

	//fin i mpi
	MPI_Finalize();

	//ok
	return EXIT_SUCCESS;
}
