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
	bool status = object.getBuffers(lst, 1000,500, ACCESS_READ);
	EXPECT_TRUE(status);
	EXPECT_EQ(1, lst.size());
}

/****************************************************/
TEST(TestObject, data_load_failure)
{
	MemoryBackendMalloc mback(NULL);
	ObjectId objectId(10, 20);
	StorageBackendGMock storage;
	Object object(&storage, &mback, objectId,1000);

	//expect call to load
	EXPECT_CALL(storage, pread(10, 20, _, 1000, 1000))
		.Times(2)
		.WillOnce(Return(-1));

	//make request with load
	ObjectSegmentList lst;
	bool statusRead = object.getBuffers(lst, 1000,500, ACCESS_READ, true, false);
	EXPECT_FALSE(statusRead);
	EXPECT_EQ(0, lst.size());

	//make request with load
	ObjectSegmentList lst2;
	bool statusWrite = object.getBuffers(lst2, 1000,500, ACCESS_READ, true, true);
	EXPECT_TRUE(statusWrite);
	EXPECT_EQ(1, lst2.size());
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

/****************************************************/
TEST(TestObject, getObejctId)
{
	MemoryBackendMalloc mback(NULL);
	ObjectId objectId(10, 20);
	Object object(NULL, &mback, objectId);

	EXPECT_EQ(objectId.high, object.getObjectId().high);
	EXPECT_EQ(objectId.low, object.getObjectId().low);
}

/****************************************************/
TEST(TestObject, setMemoryBackend)
{
	MemoryBackendMalloc mback(NULL);
	ObjectId objectId(10, 20);
	Object object(NULL, &mback, objectId);
	MemoryBackendMalloc mback2(NULL);
	object.setMemoryBackend(&mback2);
}

/****************************************************/
TEST(TestObject, forceAlignement)
{
	MemoryBackendMalloc mback(NULL);
	ObjectId objectId(10, 20);
	Object object(NULL, &mback, objectId);
	object.forceAlignement(4096);
}

/****************************************************/
TEST(TestObject, buildIovec_no_offset)
{
	//build list
	ObjectSegmentList segList;

	//fill
	char buffer[1024];
	ObjectSegmentDescr descr = {buffer, 0, 1024};
	segList.push_back(descr);

	//build iovec
	iovec * res = Object::buildIovec(segList, 0, 512);
	EXPECT_EQ((void*)buffer, (void*)(res[0].iov_base));
	EXPECT_EQ(512, res[0].iov_len);
}

/****************************************************/
TEST(TestObject, buildIovec_offset)
{
	//build list
	ObjectSegmentList segList;

	//fill
	char buffer[512];
	ObjectSegmentDescr descr = {buffer, 0, 512};
	segList.push_back(descr);

	//build iovec
	iovec * res = Object::buildIovec(segList, 128, 512);
	EXPECT_EQ((void*)(buffer+128), (void*)(res[0].iov_base));
	EXPECT_EQ(512-128, res[0].iov_len);
}
