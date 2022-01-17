/*****************************************************
*  PROJECT  : IO Catcher                             *
*  LICENSE  : Apache 2.0                             *
*  COPYRIGHT: 2020-2022 Bull SAS All rights reserved *
*****************************************************/

/****************************************************/
#include <gtest/gtest.h>
#include "../MemoryBackendNvdimmGrow.hpp"
#include <gmock/gmock.h>

/****************************************************/
using namespace IOC;
using namespace testing;

/****************************************************/
TEST(TestMemoryBackendNvdimmGrow, allocate_deallocate_no_domain)
{
	//vars
	const size_t size = 1024*1024;
	MemoryBackendNvdimmGrow backend(NULL, "/tmp/");

	//allocate
	void * ptr = backend.allocate(size);
	ASSERT_NE(nullptr, ptr);

	//deallocate
	backend.deallocate(ptr, size);
}

/****************************************************/
TEST(TestMemoryBackendNvdimmGrow, allocate_deallocate_domain)
{
	//vars
	const size_t size = 1024*1024;
	LibfabricDomain domain("localhost", "82222", true);
	MemoryBackendNvdimmGrow backend(&domain, "/tmp/");

	//allocate
	void * ptr = backend.allocate(size);
	ASSERT_NE(nullptr, ptr);

	//deallocate
	backend.deallocate(ptr, size);
}

/****************************************************/
TEST(TestMemoryBackendNvdimmGrow, allocate_deallocate_grow)
{
	//vars
	const size_t size = 1024*1024;
	LibfabricDomain domain("localhost", "82222", true);
	MemoryBackendNvdimmGrow backend(&domain, "/tmp/");

	//allocate
	void * ptr = backend.allocate(size);
	ASSERT_NE(nullptr, ptr);
	ASSERT_EQ(size, backend.getFileSize());

	//deallocate
	backend.deallocate(ptr, size);

	//allocate
	void * ptr2 = backend.allocate(size);
	ASSERT_NE(nullptr, ptr2);
	ASSERT_EQ(2*size, backend.getFileSize());

	//deallocate
	backend.deallocate(ptr2, size);
}
