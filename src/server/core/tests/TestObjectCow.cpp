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
	object.fillBuffer(1000, 500, 1);
	object.fillBuffer(2000, 500, 2);

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
	object.fillBuffer(1000, 500, 3);
	object.fillBuffer(2000, 500, 4);

	//check has cow
	ASSERT_TRUE(cowObj->checkUniq(1000, 500));
	ASSERT_TRUE(cowObj->checkUniq(2000, 500));

	//check mem content
	ASSERT_TRUE(cowObj->checkBuffer(1000, 500, 1));
	ASSERT_TRUE(cowObj->checkBuffer(2000, 500, 2));

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
	ASSERT_TRUE(cowObject.checkBuffer(1000, 500, 1));

	//check write (we must have a unir ref so keep the same buffer)
	char * ptr4 = (char*)cowObject.getUniqBuffer(1000, 500, ACCESS_WRITE, false);
	ASSERT_EQ(ptr1, ptr4);
}

/****************************************************/
TEST(TestObject, data_cow_range_simple_many)
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
		.Times(4)
		.WillRepeatedly(Return(500));
	
	//fill
	object.fillBuffer(1000, 500, 1);
	object.fillBuffer(1500, 500, 2);
	object.fillBuffer(2000, 500, 3);
	object.fillBuffer(2500, 500, 4);

	//call
	cowObject.rangeCopyOnWrite(object, 1000, 2000);

	//check cow
	char * origPtr[4];
	for (int i = 0 ; i < 4 ; i++) {
		char * ptr1 = (char*)object.getUniqBuffer(1000 + i * 500, 500, ACCESS_READ, false);
		char * ptr2 = (char*)cowObject.getUniqBuffer(1000 + i * 500, 500, ACCESS_READ, false);
		ASSERT_EQ(ptr1, ptr2);
		origPtr[i] = ptr1;
	}

	//change original segment
	object.fillBuffer(1000, 2000, 7);

	//check mem content
	for (int i = 0 ; i < 4 ; i++) {
		ASSERT_TRUE(cowObject.checkBuffer(1000 + i * 500, 500, 1 + i));

		//check write (we must have a unir ref so keep the same buffer)
		char * ptr1 = (char*)object.getUniqBuffer(1000 + i * 500, 500, ACCESS_READ, false);
		char * ptr4 = (char*)cowObject.getUniqBuffer(1000 + i * 500, 500, ACCESS_WRITE, false);
		ASSERT_NE(ptr1, ptr4);
		ASSERT_EQ(origPtr[i], ptr4);
	}
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
	ASSERT_EQ(5, v1);
	ASSERT_EQ(1, v2);

	//change oroginal segment
	object.fillBuffer(1000, 500, 3);

	//get segments
	ASSERT_TRUE(cowObject.checkUniq(1000, 500));

	//check mem content
	ASSERT_TRUE(cowObject.checkBuffer(1000, 200, 5));
	ASSERT_TRUE(cowObject.checkBuffer(1200, 100, 1));
	ASSERT_TRUE(cowObject.checkBuffer(1300, 200, 5));
}
