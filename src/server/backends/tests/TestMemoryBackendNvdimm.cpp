/*****************************************************
*  PROJECT  : IO Catcher                             *
*  LICENSE  : Apache 2.0                             *
*  COPYRIGHT: 2020-2022 Bull SAS All rights reserved *
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
TEST(TestMemoryBackendNvdimm, allocate_deallocate_grow)
{
	//vars
	const size_t size = 1024*1024;
	LibfabricDomain domain("localhost", "82222", true);
	MemoryBackendNvdimm backend(&domain, "/tmp/");

	//allocate
	void * ptr = backend.allocate(size);
	ASSERT_NE(nullptr, ptr);
	ASSERT_EQ(8*size, backend.getFileSize());
	ASSERT_EQ(1, backend.getChunks());

	//deallocate
	backend.deallocate(ptr, size);

	//allocate
	void * ptr2 = backend.allocate(size);
	ASSERT_NE(nullptr, ptr2);
	ASSERT_EQ(8*size, backend.getFileSize());
	ASSERT_EQ(1, backend.getChunks());

	//deallocate
	backend.deallocate(ptr2, size);
}

/****************************************************/
TEST(TestMemoryBackendNvdimm, allocate_deallocate_grow_2)
{
	//vars
	const size_t size = 1024*1024;
	LibfabricDomain domain("localhost", "82222", true);
	MemoryBackendNvdimm backend(&domain, "/tmp/");

	//allocate
	void * ptr[16];
	for (int i = 0 ; i < 8 ; i ++) {
		ptr[i] = backend.allocate(size);
		ASSERT_NE(nullptr, ptr);
		ASSERT_EQ(8*size, backend.getFileSize());
		ASSERT_EQ(i+1, backend.getChunks());
	}

	//allocate more
	for (int i = 8 ; i < 16 ; i ++) {
		ptr[i] = backend.allocate(size);
		ASSERT_NE(nullptr, ptr);
		ASSERT_EQ(2*8*size, backend.getFileSize());
		ASSERT_EQ(i+1, backend.getChunks());
	}

	//deallocate
	for (int i = 0 ; i < 16 ; i ++)
		backend.deallocate(ptr[i], size);
}