/*****************************************************
*  PROJECT  : IO Catcher                             *
*  LICENSE  : Apache 2.0                             *
*  COPYRIGHT: 2020-2022 Bull SAS All rights reserved *
*****************************************************/

/****************************************************/
#include <gtest/gtest.h>
#include "../MemoryBackendNvdimm.hpp"
#include "../MemoryBackendBalance.hpp"
#include <gmock/gmock.h>

/****************************************************/
using namespace IOC;
using namespace testing;

/****************************************************/
TEST(TestMemoryBackendMalloc, allocate_on_both)
{
	//vars
	const size_t size = 1024*1024;
	MemoryBackendBalance backend;

	//setup sub backends
	MemoryBackendNvdimm * backend1 = new MemoryBackendNvdimm(NULL, "/tmp/");
	MemoryBackendNvdimm * backend2 = new MemoryBackendNvdimm(NULL, "/tmp/");
	backend.registerBackend(backend1);
	backend.registerBackend(backend2);

	//allocate
	void * ptr1 = backend.allocate(size);
	ASSERT_NE(nullptr, ptr1);
	ASSERT_EQ(1, backend1->getChunks());
	ASSERT_EQ(0, backend2->getChunks());

	//allocate
	void * ptr2 = backend.allocate(size);
	ASSERT_NE(nullptr, ptr2);
	ASSERT_EQ(1, backend1->getChunks());
	ASSERT_EQ(1, backend2->getChunks());

	//deallocate
	backend.deallocate(ptr1, size);
	backend.deallocate(ptr2, size);
}

/****************************************************/
TEST(TestMemoryBackendMalloc, allocate_on_first_free)
{
	//vars
	const size_t size = 1024*1024;
	MemoryBackendBalance backend;

	//setup sub backends
	MemoryBackendNvdimm * backend1 = new MemoryBackendNvdimm(NULL, "/tmp/");
	MemoryBackendNvdimm * backend2 = new MemoryBackendNvdimm(NULL, "/tmp/");
	backend.registerBackend(backend1);
	backend.registerBackend(backend2);

	//allocate
	void * ptr1 = backend.allocate(size);
	ASSERT_NE(nullptr, ptr1);
	ASSERT_EQ(size, backend.getMem(0));
	ASSERT_EQ(1, backend1->getChunks());
	ASSERT_EQ(0, backend.getMem(1));
	ASSERT_EQ(0, backend2->getChunks());

	//deallocate
	backend.deallocate(ptr1, size);
	ASSERT_EQ(0, backend.getMem(0));
	ASSERT_EQ(0, backend1->getChunks());
	ASSERT_EQ(0, backend.getMem(1));
	ASSERT_EQ(0, backend2->getChunks());

	//allocate
	void * ptr2 = backend.allocate(size);
	ASSERT_NE(nullptr, ptr2);
	ASSERT_EQ(1, backend1->getChunks());
	ASSERT_EQ(0, backend2->getFileSize());

	//deallocate
	backend.deallocate(ptr2, size);
}
