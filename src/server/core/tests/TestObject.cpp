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

/****************************************************/
TEST(TestObject, data_cow)
{
	//spawn
	MemoryBackendMalloc mback(NULL);
	ObjectId objectId(10, 20);
	StorageBackendGMock storage;
	Object object(&storage, &mback, objectId);

	//mock
	EXPECT_CALL(storage, pread(10, 20, _, 500, _))
		.Times(2)
		.WillRepeatedly(Return(500));

	//touch ranges
	ObjectSegmentList lst1;
	object.getBuffers(lst1, 1000,500, ACCESS_WRITE);
	ObjectSegmentList lst2;
	object.getBuffers(lst2, 2000,500, ACCESS_WRITE);

	//fill
	memset(lst1.front().ptr, 1, 500);
	memset(lst2.front().ptr, 2, 500);

	//mark one as dirty
	object.markDirty(2000,500);

	//mock
	EXPECT_CALL(storage, create(10, 21));
	EXPECT_CALL(storage, makeCowSegment(10, 20, 10, 21, 0, 1000))
		.Times(1)
		.WillOnce(Return(1000));
	EXPECT_CALL(storage, makeCowSegment(10, 20, 10, 21, 1500, 500))
		.Times(1)
		.WillOnce(Return(500));(Return(500));
	EXPECT_CALL(storage, pwrite(10, 21, _, 500, 1000))
		.Times(1)
		.WillOnce(Return(500));

	//call
	ObjectId cowId(10, 21);
	Object * cowObj = object.makeFullCopyOnWrite(cowId, true);

	//change oroginal segment
	ObjectSegmentList lstorig1;
	object.getBuffers(lstorig1, 1000,500, ACCESS_WRITE);
	ObjectSegmentList lstorig2;
	object.getBuffers(lstorig2, 2000,500, ACCESS_WRITE);
	memset(lstorig1.front().ptr, 3, 500);
	memset(lstorig2.front().ptr, 4, 500);

	//get segments
	ObjectSegmentList lstdest1;
	cowObj->getBuffers(lstdest1, 1000,500, ACCESS_READ);
	ObjectSegmentList lstdest2;
	cowObj->getBuffers(lstdest2, 2000,500, ACCESS_READ);

	//check mem content
	char * ptr1 = (char*)lstdest1.front().ptr;
	char * ptr2 = (char*)lstdest2.front().ptr;
	ASSERT_NE(ptr1, ptr2);
	for (size_t i = 0 ; i < 500 ; i++) {
		ASSERT_EQ(1, ptr1[i]) << "Index " << i;
		ASSERT_EQ(2, ptr2[i]) << "Index " << i;
	}

	delete cowObj;
}

/****************************************************/
TEST(TestObject, data_cow_range_simple)
{
	//spawn
	MemoryBackendMalloc mback(NULL);
	ObjectId objectId1(10, 20);
	StorageBackendGMock storage;
	Object object(&storage, &mback, objectId1);

	//cow object
	ObjectId objectId2(10, 21);
	Object cowObject(&storage, &mback, objectId2);

	//mock
	EXPECT_CALL(storage, pread(10, 20, _, 500, _))
		.Times(1)
		.WillRepeatedly(Return(500));
	
	//fill
	object.fillBuffer(1000, 500, 1);

	//call
	cowObject.rangeCopyOnWrite(object, 1000, 500);

	//check cow
	char * ptr1 = (char*)object.getUniqBuffer(1000, 500, ACCESS_READ, false);
	char * ptr2 = (char*)cowObject.getUniqBuffer(1000, 500, ACCESS_READ, false);
	ASSERT_EQ(ptr1, ptr2);

	//change oroginal segment
	object.fillBuffer(1000, 500, 3);

	//check mem content
	char * ptr3 = (char*)cowObject.getUniqBuffer(1000, 500, ACCESS_READ, false);
	ASSERT_TRUE(cowObject.checkBuffer(1000, 500, 1));

	//check write (we must have a unir ref so keep the same buffer)
	char * ptr4 = (char*)cowObject.getUniqBuffer(1000, 500, ACCESS_WRITE, false);
	ASSERT_EQ(ptr1, ptr4);
}

/****************************************************/
TEST(TestObject, data_cow_range_not_full_overlap)
{
	//spawn
	MemoryBackendMalloc mback(NULL);
	ObjectId objectId1(10, 20);
	StorageBackendGMock storage;
	Object object(&storage, &mback, objectId1);

	//cow object
	ObjectId objectId2(10, 21);
	Object cowObject(&storage, &mback, objectId2);

	//mock
	EXPECT_CALL(storage, pread(10, 20, _, 500, _))
		.Times(1)
		.WillRepeatedly(Return(500));

	//touch range on orig
	object.fillBuffer(1000, 500, 1);

	//mock
	EXPECT_CALL(storage, pread(10, 21, _, 100, _))
		.Times(1)
		.WillRepeatedly(Return(100));

	//call
	cowObject.rangeCopyOnWrite(object, 1200, 100);

	//change oroginal segment
	object.fillBuffer(1000, 500, 3);

	//get segments
	ASSERT_TRUE(cowObject.checkUniq(1200, 100));

	//check mem content
	ASSERT_TRUE(cowObject.checkBuffer(1200, 100, 1));
}

/****************************************************/
TEST(TestObject, data_cow_range_not_full_overlap_preexist)
{
	//spawn
	MemoryBackendMalloc mback(NULL);
	ObjectId objectId1(10, 20);
	StorageBackendGMock storage;
	Object object(&storage, &mback, objectId1);

	//cow object
	ObjectId objectId2(10, 21);
	Object cowObject(&storage, &mback, objectId2);

	//mock
	EXPECT_CALL(storage, pread(10, 20, _, 500, _))
		.Times(1)
		.WillRepeatedly(Return(500));

	//touch range on orig
	object.fillBuffer(1000, 500, 1);

	//mock
	EXPECT_CALL(storage, pread(10, 21, _, 500, _))
		.Times(1)
		.WillRepeatedly(Return(500));
	
	//make prexisting non fully overlap range
	cowObject.fillBuffer(1000, 500, 5);
	ASSERT_TRUE(cowObject.checkUniq(1000, 500));

	//call
	char v1 = cowObject.getUniqBuffer(1000, 500, ACCESS_READ, false)[200];
	cowObject.rangeCopyOnWrite(object, 1200, 100);
	char v2 = cowObject.getUniqBuffer(1000, 500, ACCESS_READ, false)[200];

	//change oroginal segment
	object.fillBuffer(1000, 500, 3);

	//get segments
	ASSERT_TRUE(cowObject.checkUniq(1000, 500));

	//check mem content
	ASSERT_TRUE(cowObject.checkBuffer(1000, 200, 5));
	ASSERT_TRUE(cowObject.checkBuffer(1200, 100, 1));
	ASSERT_TRUE(cowObject.checkBuffer(1300, 200, 5));
}
