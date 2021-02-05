#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <ummap/ummap.h>
#include <ioc-client.h>
#include <time.h>
#include <assert.h>

//gcc -g main-demo.c -I$HOME/test-rdma/usr2/include -L$HOME/test-rdma/usr2/lib -L$HOME/test-rdma/usr2/lib64 -lummap-io -lioc-client -lfabric -o demo
//gcc -g main-demo.c -I$HOME/test-rdma/usr2/include -L$HOME/test-rdma/usr2/lib -L$HOME/test-rdma/usr2/lib64 -lummap-io -lioc-client -lfabric -o demo -fopenmp

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

int main(int argc, char ** argv)
{
	//errors
	if (argc != 3) {
		printf("Uage: a.out {IP} {SEG_SIZE}\n");
		return EXIT_FAILURE;
	}

	//connect to server
	ioc_client_t * client = ioc_client_init(argv[1], "8556");
	printf("Provider: %s\n", ioc_client_provider_name(client));

	//init ummap
	ummap_init();

	//setup uri system
	ummap_config_ioc_init_options(argv[1], "8556");

	//create driver
	//ummap_driver_t * driver = ummap_driver_create_ioc(client, 10, 22, true);
	ummap_driver_t * driver = ummap_driver_create_uri("ioc://10:22");
	ummap_driver_set_autoclean(driver, false);

	printf("\n========================= basics ============================\n");

	//map
	size_t size = 20*1024*1024;
	char * ptr = ummap(NULL, size, 256*1024, 0, PROT_READ|PROT_WRITE, 0, driver, NULL, NULL);
	//char * ptr = ummap(NULL, size, 256*1024, 0, PROT_READ|PROT_WRITE, UMMAP_THREAD_UNSAFE, driver, NULL, NULL);
	//ummap_skip_first_read(ptr);

	//read
	char sum = 0;
	int k = 0;
	printf("Read...\n");
	#pragma omp parallel for
	for (k = 0 ; k < size ; k++)
		sum += ptr[k];

	//set
	printf("Write...\n");
	memset(ptr, 'a', size);
	//printf("%s\n", ptr);

	//sync
	printf("Sync...\n");
	umsync(ptr, size, 0);

	//unmap
	umunmap(ptr, 0);

	//--------workflow-------
	printf("workflow\n");
	ptr = ummap(NULL, size, 256*1024, 0, PROT_READ|PROT_WRITE, UMMAP_NO_FIRST_READ, driver, NULL, NULL);
	memset(ptr, 5, size);
	umunmap(ptr, true);
	ptr = ummap(NULL, size, 256*1024, 0, PROT_READ|PROT_WRITE, 0, driver, NULL, NULL);
	for (k = 0 ; k < size ; k++)
		assert(ptr[k] == 5);
	umunmap(ptr, false);

	//--------bench--------
	//time
	char * p = malloc(size);
	memset(p, 0, size);

	//set params
	size_t repeat = 3000;
	size_t repeat2 = 700;
	size_t segSize = atol(argv[2])*1024;
	size_t tmp = 0;
	int i,j;
	printf("Seg_size: %zu\n", segSize);

	//bench
	printf("\n==================== Bench ummap read =======================\n");
	/*printf("-------------------------------------------------------------\n");
	printf("ptr = ummap(....);\n");
	printf("#pragma omp parallel for\n");
	printf("for (j = 0 ; j < size ; j+=segSize)\n");
	printf("\ttmp += ptr[j];\n");
	printf("-------------------------------------------------------------\n");*/
	//printf("Bench ummap read...\n");
	struct timespec start, stop;
	clock_gettime(CLOCK_MONOTONIC, &start);
	
	for (i = 0 ; i < repeat ; i++) {
		ptr = ummap(NULL, size, segSize, 0, PROT_READ|PROT_WRITE, 0, driver, NULL, NULL);
		#pragma omp parallel for
		for (j = 0 ; j < size ; j+=segSize)
			tmp += ptr[j];
		umunmap(ptr, 0);
	}


	//time
	size_t cnt = repeat * size / segSize;
	size_t tot = repeat * size;
	clock_gettime(CLOCK_MONOTONIC, &stop);
	double result = timespec_diff(&stop, &start);
	double rate = (double)cnt / result / 1000.0;
	double bandwidth = 8.0 * (double)tot / result / 1000.0 / 1000.0 / 1000.0;
	double bandwidth2 = (double)tot / result / 1024.0 / 1024.0 / 1024.0;
	printf("Average bandwidth: %g GBits/s, %g GBytes/s\n", bandwidth, bandwidth2);

	//-------------------

	//bench
	printf("\n==================== Bench ummap write ======================\n");
	/*printf("-------------------------------------------------------------\n");
	printf("ptr = ummap(....UMMAP_NO_FIRST_READ....);\n");
	printf("#pragma omp parallel for\n");
	printf("for (j = 0 ; j < size ; j+=segSize)\n");
	printf("\tptr[j] += 1;\n");
	printf("umsync(ptr, 0, false);\n");
	printf("-------------------------------------------------------------\n");*/
	//printf("Bench ummap write...\n");
	clock_gettime(CLOCK_MONOTONIC, &start);
	
	for (i = 0 ; i < repeat2 ; i++) {
		ptr = ummap(NULL, size, segSize, 0, PROT_READ|PROT_WRITE, UMMAP_NO_FIRST_READ, driver, NULL, NULL);
		for (j = 0 ; j < size ; j+=segSize)
			ptr[j] += 1;
		umsync(ptr, 0, false);
		umunmap(ptr, false);
	}


	//time
	cnt = repeat2 * size / segSize;
	tot = repeat2 * size;
	clock_gettime(CLOCK_MONOTONIC, &stop);
	result = timespec_diff(&stop, &start);
	rate = (double)cnt / result / 1000.0;
	bandwidth = 8.0 * (double)tot / result / 1000.0 / 1000.0 / 1000.0;
	bandwidth2 = (double)tot / result / 1024.0 / 1024.0 / 1024.0;
	printf("Average bandwidth: %g GBits/s, %g GBytes/s\n", bandwidth, bandwidth2);

	//-------------------

	//bench
	printf("\n==================== Bench ummap dummy ======================\n");
	/*printf("-------------------------------------------------------------\n");
	printf("ptr = ummap(...);\n");
	printf("#pragma omp parallel for\n");
	printf("for (j = 0 ; j < size ; j+=segSize)\n");
	printf("\ttmp += ptr[j];\n");
	printf("-------------------------------------------------------------\n");*/
	//printf("Bench ummap dummy...\n");
	clock_gettime(CLOCK_MONOTONIC, &start);
	
	for (i = 0 ; i < repeat ; i++) {
		ummap_driver_t * d = ummap_driver_create_dummy(0);
		ptr = ummap(NULL, size, segSize, 0, PROT_READ|PROT_WRITE, 0, d, NULL, NULL);
		#pragma omp parallel for
		for (j = 0 ; j < size ; j+=segSize)
			tmp += ptr[j];
		umunmap(ptr, false);
	}


	//time
	cnt = repeat * size / segSize;
	tot = repeat * size;
	clock_gettime(CLOCK_MONOTONIC, &stop);
	result = timespec_diff(&stop, &start);
	rate = (double)cnt / result / 1000.0;
	bandwidth = 8.0 * (double)tot / result / 1000.0 / 1000.0 / 1000.0;
	bandwidth2 = (double)tot / result / 1024.0 / 1024.0 / 1024.0;
	printf("Average bandwidth: %g GBits/s, %g GBytes/s\n", bandwidth, bandwidth2);

	//-------------------
	printf("\n==================== Bench direct read ======================\n");
	/*printf("-------------------------------------------------------------\n");
	printf("#pragma omp for\n");
	printf("for (i = 0 ; i < repeat ; i++) {\n");
	printf("\tfor (j = 0 ; j < size ; j+=segSize)\n");
	printf("\t\tioc_client_obj_read(client, 10, 20, p + j, segSize, 0);\n");
	printf("-------------------------------------------------------------\n");*/
	//printf("Bench direct read...\n");
	repeat = 4000;

	clock_gettime(CLOCK_MONOTONIC, &start);

	#pragma omp parallel
	{
		void * p = malloc(size);
		#pragma omp for
		for (i = 0 ; i < repeat ; i++) {
			for (j = 0 ; j < size ; j+=segSize)
				ioc_client_obj_read(client, 10, 20, p + j, segSize, 0);
		}
		free(p);
	}

	//time
	cnt = repeat * size / segSize;
	tot = repeat * size;
	clock_gettime(CLOCK_MONOTONIC, &stop);
	result = timespec_diff(&stop, &start);
	rate = (double)cnt / result / 1000.0;
	bandwidth = 8.0 * (double)tot / result / 1000.0 / 1000.0 / 1000.0;
	bandwidth2 = (double)tot / result / 1024.0 / 1024.0 / 1024.0;
	printf("Average bandwidth: %g GBits/s, %g GBytes/s\n", bandwidth, bandwidth2);

	//-------------------
	printf("\n==================== Bench direct write =====================\n");
	/*printf("-------------------------------------------------------------\n");
	printf("#pragma omp for\n");
	printf("for (i = 0 ; i < repeat ; i++) {\n");
	printf("\tfor (j = 0 ; j < size ; j+=segSize)\n");
	printf("\t\tioc_client_obj_write(client, 10, 20, p + j, segSize, 0);\n");
	printf("-------------------------------------------------------------\n");*/
	//printf("Bench direct write...\n");

	clock_gettime(CLOCK_MONOTONIC, &start);

	#pragma omp parallel
	{
		void * p = malloc(size);
		#pragma omp for
		for (i = 0 ; i < repeat2 ; i++) {
			for (j = 0 ; j < size ; j+=segSize)
				ioc_client_obj_write(client, 10, 20, p + j, segSize, 0);
		}
		free(p);
	}

	//time
	cnt = repeat2 * size / segSize;
	tot = repeat2 * size;
	clock_gettime(CLOCK_MONOTONIC, &stop);
	result = timespec_diff(&stop, &start);
	rate = (double)cnt / result / 1000.0;
	bandwidth = 8.0 * (double)tot / result / 1000.0 / 1000.0 / 1000.0;
	bandwidth2 = (double)tot / result / 1024.0 / 1024.0 / 1024.0;
	printf("Average bandwidth: %g GBits/s, %g GBytes/s\n", bandwidth, bandwidth2);

	//finalize ummap
	ummap_finalize();
	
	//clean connection
	ioc_client_fini(client);
}
