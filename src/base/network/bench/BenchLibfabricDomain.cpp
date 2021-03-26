/*****************************************************
			PROJECT  : IO Catcher
			LICENSE  : Apache 2.0
			COPYRIGHT: 2020 Bull SAS
*****************************************************/

/****************************************************/
#include <cstdlib>
#include <x86intrin.h>
#include "../LibfabricDomain.hpp"

/****************************************************/
using namespace IOC;

/****************************************************/
void benchSegmentRegistration(void)
{
	//vars
	uint64_t start;
	uint64_t stop;

	//create a domain
	LibfabricDomain domain("127.0.0.1", "8555", true);

	//allocate memory
	const size_t cnt = 16*1024;
	const size_t size = 4096;
	void ** ptr = new void*[cnt];
	for (size_t i = 0 ; i < cnt ; i++)
		ptr[i] = malloc(size);
	printf("Insertions                      : %lu\n", cnt);

	//register
	start = _rdtsc();
	for (size_t i = 0 ; i < cnt ; i++)
		domain.registerSegment(ptr[i], size, true, true, false);
	stop = _rdtsc();

	//print
	printf("Average time per registerSegment: %0.01f Cycles\n", (float)(stop - start) / (float)cnt);

	//getFidMR
	start = _rdtsc();
	for (size_t i = 0 ; i < cnt ; i++)
		domain.getFidMR((char*)(ptr[i]) + 1024, 1024);
	stop = _rdtsc();

	//print
	printf("Average time per getFidMR       : %0.01f Cycles\n", (float)(stop - start) / (float)cnt);

	//clear mem
	delete [] ptr;
}

/****************************************************/
int main(void)
{
	//bench 1
	printf("================== Segment registration =====================\n");
	benchSegmentRegistration();

	//ok
	return EXIT_SUCCESS;
}
