/*****************************************************
			PROJECT  : IO Catcher
			LICENSE  : Apache 2.0
			COPYRIGHT: 2020 Bull SAS
*****************************************************/

/****************************************************/
#include <gtest/gtest.h>
#include "../Object.hpp"
#include "../../backends/StorageBackendGMock.hpp"
#include "../../backends/MemoryBackendMalloc.hpp"

/****************************************************/
using namespace IOC;
using namespace testing;

/****************************************************/
TEST(TestObject, getBuffers_1)
{
	MemoryBackendMalloc mback(NULL);
	ObjectId objectId(10, 20);
	Object object(NULL, &mback, objectId);

	ObjectSegmentList lst;
	object.getBuffers(lst, 1000,500, ACCESS_READ);
	EXPECT_EQ(1, lst.size());
}

/****************************************************/
TEST(TestObject, getBuffers_2)
{
	MemoryBackendMalloc mback(NULL);
	ObjectId objectId(10, 20);
	Object object(NULL, &mback, objectId);

	ObjectSegmentList lst;
	object.getBuffers(lst, 1000,500, ACCESS_READ);
	EXPECT_EQ(1, lst.size());

	lst.clear();
	object.getBuffers(lst, 1000,1000, ACCESS_READ);
	EXPECT_EQ(2, lst.size());
}

/****************************************************/
TEST(TestObject, getBuffers_3)
{
	MemoryBackendMalloc mback(NULL);
	ObjectId objectId(10, 20);
	Object object(NULL, &mback, objectId);

	ObjectSegmentList lst;
	object.getBuffers(lst, 1000,300, ACCESS_READ);
	EXPECT_EQ(1, lst.size());

	lst.clear();
	object.getBuffers(lst, 1600,300, ACCESS_READ);
	EXPECT_EQ(1, lst.size());

	lst.clear();
	object.getBuffers(lst, 1000,900, ACCESS_READ);
	EXPECT_EQ(3, lst.size());
}

/****************************************************/
TEST(TestObject, getBuffers_4)
{
	MemoryBackendMalloc mback(NULL);
	ObjectId objectId(10, 20);
	Object object(NULL, &mback, objectId);

	ObjectSegmentList lst;
	object.getBuffers(lst, 1000,500, ACCESS_READ);
	EXPECT_EQ(1, lst.size());

	lst.clear();
	object.getBuffers(lst, 800,1800, ACCESS_READ);
	EXPECT_EQ(3, lst.size());
}

/****************************************************/
TEST(TestObject, getBuffers_5_alignement)
{
	MemoryBackendMalloc mback(NULL);
	ObjectId objectId(10, 20);
	Object object(NULL, &mback, objectId, 1000);

	ObjectSegmentList lst;
	object.getBuffers(lst, 1000,500, ACCESS_READ);
	EXPECT_EQ(1, lst.size());

	lst.clear();
	object.getBuffers(lst, 1000,1000, ACCESS_READ);
	EXPECT_EQ(1, lst.size());
}

/****************************************************/
TEST(TestObject, getBuffers_6_alignement)
{
	MemoryBackendMalloc mback(NULL);
	ObjectId objectId(10, 20);
	Object object(NULL, &mback, objectId, 1000);

	ObjectSegmentList lst;
	object.getBuffers(lst, 1000,500, ACCESS_READ);
	EXPECT_EQ(1, lst.size());

	lst.clear();
	object.getBuffers(lst, 1000,1500, ACCESS_READ);
	EXPECT_EQ(2, lst.size());
}

/****************************************************/
TEST(TestObject, getBuffers_7_check_addr)
{
	MemoryBackendMalloc mback(NULL);
	ObjectId objectId(10, 20);
	Object object(NULL, &mback, objectId, 0);

	ObjectSegmentList lst;
	object.getBuffers(lst, 1000,500, ACCESS_READ);
	EXPECT_EQ(1, lst.size());
	void * ptr1 = lst.front().ptr;

	ObjectSegmentList lst2;
	object.getBuffers(lst2, 1200,1000, ACCESS_READ);
	EXPECT_EQ(2, lst2.size());

	void * ptr2 = lst2.front().ptr;
	EXPECT_EQ(ptr1, ptr2);
}

/****************************************************/
TEST(TestObject, getBuffers_8_check_addr_inner)
{
	MemoryBackendMalloc mback(NULL);
	ObjectId objectId(10, 20);
	Object object(NULL, &mback, objectId, 0);

	ObjectSegmentList lst;
	object.getBuffers(lst, 1000,500, ACCESS_READ);
	EXPECT_EQ(1, lst.size());
	void * ptr1 = lst.front().ptr;

	ObjectSegmentList lst2;
	object.getBuffers(lst2, 1200,100, ACCESS_READ);
	EXPECT_EQ(1, lst2.size());

	void * ptr2 = lst2.front().ptr;
	EXPECT_EQ(ptr1, ptr2);
}

/****************************************************/
TEST(TestObject, data_load)
{
	MemoryBackendMalloc mback(NULL);
	ObjectId objectId(10, 20);
	StorageBackendGMock storage;
	Object object(&storage, &mback, objectId);

	//expect call to load
	EXPECT_CALL(storage, pread(10, 20, _, 500, 1000))
		.Times(1)
		.WillOnce(Return(500));

	//make request
	ObjectSegmentList lst;
	object.getBuffers(lst, 1000,500, ACCESS_READ);
	EXPECT_EQ(1, lst.size());
}

/****************************************************/
TEST(TestObject, data_create)
{
	MemoryBackendMalloc mback(NULL);
	ObjectId objectId(10, 20);
	StorageBackendGMock storage;
	Object object(&storage, &mback, objectId);

	//expect object create
	EXPECT_CALL(storage, create(10,20))
		.Times(1)
		.WillOnce(Return(0));
	
	//call
	object.create();
}

/****************************************************/
TEST(TestObject, data_flush)
{
	MemoryBackendMalloc mback(NULL);
	ObjectId objectId(10, 20);
	StorageBackendGMock storage;
	Object object(&storage, &mback, objectId);

	//expect call to load
	EXPECT_CALL(storage, pread(10, 20, _, 500, 1000))
		.Times(1)
		.WillOnce(Return(500));

	//make request
	ObjectSegmentList lst;
	object.getBuffers(lst, 1000,500, ACCESS_READ);
	EXPECT_EQ(1, lst.size());

	//nothing to flush
	object.flush(0,0);

	//mark dirty
	object.markDirty(1000, 500);

	//expect call to write
	EXPECT_CALL(storage, pwrite(10, 20, _, 500, 1000))
		.Times(1)
		.WillOnce(Return(500));

	//flush
	object.flush(0,0);
}
