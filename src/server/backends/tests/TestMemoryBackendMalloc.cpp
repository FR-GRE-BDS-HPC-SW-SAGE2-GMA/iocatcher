/*****************************************************
			PROJECT  : IO Catcher
			LICENSE  : Apache 2.0
			COPYRIGHT: 2020 Bull SAS
*****************************************************/

/****************************************************/
#include <gtest/gtest.h>
#include "../MemoryBackendMalloc.hpp"
#include <gmock/gmock.h>

/****************************************************/
using namespace IOC;
using namespace testing;

/****************************************************/
TEST(TestMemoryBackendMalloc, allocate_deallocate_no_domain)
{
	//vars
	const size_t size = 1024*1024;
	MemoryBackendMalloc backend(NULL);

	//allocate
	void * ptr = backend.allocate(size);
	ASSERT_NE(nullptr, ptr);

	//deallocate
	backend.deallocate(ptr, size);
}

/****************************************************/
TEST(TestMemoryBackendMalloc, allocate_deallocate_domain)
{
	//vars
	const size_t size = 1024*1024;
	LibfabricDomain domain("localhost", "82222", true);
	MemoryBackendMalloc backend(&domain);

	//allocate
	void * ptr = backend.allocate(size);
	ASSERT_NE(nullptr, ptr);

	//deallocate
	backend.deallocate(ptr, size);
}
