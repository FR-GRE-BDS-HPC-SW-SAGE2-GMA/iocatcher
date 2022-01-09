/*****************************************************
			PROJECT  : IO Catcher
			VERSION  : 0.0.0-dev
			DATE     : 10/2020
			LICENSE  : ????????
*****************************************************/

/****************************************************/
#include <cstdlib>
#include <cstring>
#include <x86intrin.h>
#include "../Hook.hpp"
#include "../Serializer.hpp"

/****************************************************/
using namespace IOC;

/****************************************************/
void runSerialize(LibfabricObjReadWriteInfos & objReadWrite, char * buffer, size_t size)
{
	Serializer serializer(buffer, size);
	serializer.apply("objReadWrite", objReadWrite);
}

/****************************************************/
void runDeSerialize(LibfabricObjReadWriteInfos & objReadWrite, char * buffer, size_t size)
{
	DeSerializer deserializer(buffer, size);
	deserializer.apply("objReadWrite", objReadWrite);
}

/****************************************************/
void benchSerialization(void)
{
	//vars
	uint64_t start;
	uint64_t stop;
	const size_t cnt = 2048*1024;

	//serialize a struct
	char data[1024];
	memset(data, 1, 1024);
	LibfabricObjReadWriteInfos objReadWrite = {
		.objectId = {
			.low = 10,
			.high = 20,
		},
		.offset = 0,
		.size = 1024,
		.msgHasData = true,
		.optionalData = data
	};

	//serialize
	char buffer[2048];
	start = _rdtsc();
	for (size_t i = 0 ; i < cnt ; i++) {
		runSerialize(objReadWrite, buffer, sizeof(buffer));
	}
	stop = _rdtsc();

	//print
	printf("Average time per serialize  : %0.01f Cycles\n", (float)(stop - start) / (float)cnt);

	//getFidMR
	LibfabricObjReadWriteInfos objReadWriteOut;
	start = _rdtsc();
	for (size_t i = 0 ; i < cnt ; i++)
		runDeSerialize(objReadWriteOut, buffer, sizeof(buffer));
	stop = _rdtsc();

	//print
	printf("Average time per deserialize: %0.01f Cycles\n", (float)(stop - start) / (float)cnt);
}

/****************************************************/
int main(void)
{
	//bench 1
	printf("================== Serializer =====================\n");
	benchSerialization();

	//ok
	return EXIT_SUCCESS;
}
