/*****************************************************
			PROJECT  : IO Catcher
			LICENSE  : Apache 2.0
			COPYRIGHT: 2020 Bull SAS
*****************************************************/

/****************************************************/
#include <gtest/gtest.h>
#include "../Object.hpp"
#include "../../backends/StorageBackendGMock.hpp"

/****************************************************/
using namespace IOC;
using namespace testing;

/****************************************************/
TEST(TestObjectSegment, overlap)
{
	//build
	size_t offset = 1000;
	size_t size = 500;
	ObjectSegment seg(offset, size, NULL, false);

	//check cases
	EXPECT_TRUE(seg.overlap(1000,500));
	EXPECT_TRUE(seg.overlap(800,500));
	EXPECT_TRUE(seg.overlap(1200,500));
	EXPECT_FALSE(seg.overlap(1500,500));
	EXPECT_FALSE(seg.overlap(500,500));
}

/****************************************************/
TEST(TestObject, getBuffers_1)
{
	ObjectId objectId(10, 20);
	Object object(NULL, NULL, objectId);

	ObjectSegmentList lst;
	object.getBuffers(lst, 1000,500, ACCESS_READ);
	EXPECT_EQ(1, lst.size());
}

/****************************************************/
TEST(TestObject, getBuffers_2)
{
	ObjectId objectId(10, 20);
	Object object(NULL, NULL, objectId);

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
	ObjectId objectId(10, 20);
	Object object(NULL, NULL, objectId);

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
	ObjectId objectId(10, 20);
	Object object(NULL, NULL, objectId);

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
	ObjectId objectId(10, 20);
	Object object(NULL, NULL, objectId, 1000);

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
	ObjectId objectId(10, 20);
	Object object(NULL, NULL, objectId, 1000);

	ObjectSegmentList lst;
	object.getBuffers(lst, 1000,500, ACCESS_READ);
	EXPECT_EQ(1, lst.size());

	lst.clear();
	object.getBuffers(lst, 1000,1500, ACCESS_READ);
	EXPECT_EQ(2, lst.size());
}

/****************************************************/
TEST(TestObject, data_load)
{
	ObjectId objectId(10, 20);
	StorageBackendGMock storage;
	Object object(&storage, NULL, objectId);

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
	ObjectId objectId(10, 20);
	StorageBackendGMock storage;
	Object object(&storage, NULL, objectId);

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
	ObjectId objectId(10, 20);
	StorageBackendGMock storage;
	Object object(&storage, NULL, objectId);

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
	ObjectId objectId(10, 20);
	StorageBackendGMock storage;
	Object object(&storage, NULL, objectId);

	//mock
	EXPECT_CALL(storage, pread(10, 20, _, 500, _))
		.Times(2)
		.WillRepeatedly(Return(500));

	//touch gangres
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
	Object * cowObj = object.makeCopyOnWrite(cowId, true);

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
