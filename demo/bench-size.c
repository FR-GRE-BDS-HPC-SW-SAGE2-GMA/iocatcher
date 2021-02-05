#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <ummap/ummap.h>
#include <ioc-client.h>
#include <time.h>

int main(int argc, char ** argv)
{
	//connect to server
	struct ioc_client_t * client = ioc_client_init(argv[1], "8556");

	//init ummap
	ummap_init();

	//create driver
	ummap_driver_t * driver = ummap_driver_create_ioc(client, 10, 22, true);

	//map
	size_t size = 20*1024*1024;
	char * ptr = ummap(NULL, size, 256*1024, 0, PROT_READ|PROT_WRITE, 0, driver, NULL, NULL);
	ummap_skip_first_read(ptr);

	//read
	char sum = 0;
	int k = 0;
	printf("Read...\n");
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

	//--------bench--------
	printf("Bench ummap...\n");
	//time
	char * p = malloc(size);
	memset(p, 0, size);
	struct timespec start, stop;
	clock_gettime(CLOCK_MONOTONIC, &start);

	size_t repeat = 1000;
	size_t segSize = 1*1024*1024;
	size_t tmp = 0;
	int i,j;
	
	/*for (i = 0 ; i < repeat ; i++) {
		ptr = ummap(size, segSize, 0, PROT_READ|PROT_WRITE, 0, driver, NULL, NULL);
		for (j = 0 ; j < size ; j+=segSize)
			tmp += ptr[j];
		umunmap(ptr, 0);
	}*/


	//time
	size_t cnt = repeat * size / segSize;
	size_t tot = repeat * size;
	clock_gettime(CLOCK_MONOTONIC, &stop);
	double result = (stop.tv_sec - start.tv_sec) + (stop.tv_nsec - start.tv_nsec) / 1e9;    // in microseconds
	double rate = (double)cnt / result / 1000.0;
	double bandwidth = 8.0 * (double)tot / result / 1000.0 / 1000.0 / 1000.0;
	printf("Time: %g s, rate: %g kOPS, bandwidth: %g GBits/s, size: %zu\n", result, rate, bandwidth, segSize);
	//bandwidth = (double)tot / result / 1024.0 / 1024.0;
	//printf("Time: %g s, rate: %g kOPS, bandwidth: %g MBytes/s, size: %zu\n", result, rate, bandwidth, segSize);

	//-------------------
	printf("Bench direct...\n");

	int u;
	for (u = 64 ; u *= 2 ; u <= 4*1024*1024)
	{
		segSize = u;
		if (u >= 1024*1024)
			segSize = 1024*1024;
		size = u;
		repeat = 10;
		if (u < 256*4096)
			repeat = 10000;
		if (u < 8*4096)
			repeat = 100000;
		if (u < 4096)
			repeat = 1000000;
		clock_gettime(CLOCK_MONOTONIC, &start);

		for (i = 0 ; i < repeat ; i++)
			for (j = 0 ; j < size ; j+=segSize)
				ioc_client_obj_read(client, 10, 20, p + j, segSize, 0);


		//time
		cnt = repeat * size / segSize;
		tot = repeat * size;
		clock_gettime(CLOCK_MONOTONIC, &stop);
		result = (stop.tv_sec - start.tv_sec) + (stop.tv_nsec - start.tv_nsec) / 1e9;    // in microseconds
		rate = (double)cnt / result / 1000.0;
		bandwidth = 8.0 * (double)tot / result / 1000.0 / 1000.0 / 1000.0;
		printf("Time: %g s, rate: %g kOPS, bandwidth: %g GBits/s, size: %zu\n", result, rate, bandwidth, size);
		//bandwidth = (double)tot / result / 1024.0 / 1024.0;
		//printf("Time: %g s, rate: %g kOPS, bandwidth: %g MBytes/s, size: %zu\n", result, rate, bandwidth, segSize);

	}

	//finalize ummap
	ummap_finalize();
	
	//clean connection
	ioc_client_fini(client);
}
