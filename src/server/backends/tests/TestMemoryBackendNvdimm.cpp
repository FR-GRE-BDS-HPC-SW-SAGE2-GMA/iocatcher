/*****************************************************
			PROJECT  : IO Catcher
			LICENSE  : Apache 2.0
			COPYRIGHT: 2020 Bull SAS
*****************************************************/

/****************************************************/
#include <gtest/gtest.h>
#include "../MemoryBackendNvdimm.hpp"
#include <gmock/gmock.h>

/****************************************************/
using namespace IOC;
using namespace testing;

/****************************************************/
TEST(TestMemoryBackendNvdimm, allocate_deallocate_no_domain)
{
	//vars
	const size_t size = 1024*1024;
	MemoryBackendNvdimm backend(NULL, "/tmp/");

	//allocate
	void * ptr = backend.allocate(size);
	ASSERT_NE(nullptr, ptr);

	//deallocate
	backend.deallocate(ptr, size);
}

/****************************************************/
TEST(TestMemoryBackendNvdimm, allocate_deallocate_domain)
{
	//vars
	const size_t size = 1024*1024;
	LibfabricDomain domain("localhost", "82222", true);
	MemoryBackendNvdimm backend(&domain, "/tmp/");

	//allocate
	void * ptr = backend.allocate(size);
	ASSERT_NE(nullptr, ptr);

	//deallocate
	backend.deallocate(ptr, size);
}

/****************************************************/
TEST(TestMemoryBackendNvdimm, mem_reuse_1)
{
	//vars
	const size_t size = 1024*1024;
	LibfabricDomain domain("localhost", "82222", true);
	MemoryBackendNvdimm backend(&domain, "/tmp/");

	//allocate
	void * ptr = backend.allocate(size);
	ASSERT_NE(nullptr, ptr);

	//deallocate
	backend.deallocate(ptr, size);

	//allocate
	void * ptr2 = backend.allocate(size);
	ASSERT_EQ(ptr, ptr2);
	backend.deallocate(ptr2, size);
}

/****************************************************/
TEST(TestMemoryBackendNvdimm, mem_reuse_2)
{
	//vars
	const size_t size = 1024*1024;
	LibfabricDomain domain("localhost", "82222", true);
	MemoryBackendNvdimm backend(&domain, "/tmp/");

	//allocate
	void * ptr1 = backend.allocate(size); ASSERT_NE(nullptr, ptr1);
	void * ptr2 = backend.allocate(size); ASSERT_NE(nullptr, ptr2);
	void * ptr3 = backend.allocate(size); ASSERT_NE(nullptr, ptr3);
	void * ptr4 = backend.allocate(size); ASSERT_NE(nullptr, ptr4);

	//deallocate
	backend.deallocate(ptr2, size);
	backend.deallocate(ptr3, size);

	//allocate
	void * ptr5 = backend.allocate(size);
	ASSERT_EQ(ptr3, ptr5);
	void * ptr6 = backend.allocate(size);
	ASSERT_EQ(ptr2, ptr6);

	//deallocate
	backend.deallocate(ptr1, size);
	backend.deallocate(ptr2, size);
	backend.deallocate(ptr5, size);
	backend.deallocate(ptr4, size);
}
