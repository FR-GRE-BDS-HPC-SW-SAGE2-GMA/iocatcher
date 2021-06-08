/*****************************************************
            PROJECT  : IO Catcher
            LICENSE  : Apache 2.0
			COPYRIGHT: 2020 Bull SAS
COPYRIGHT: 2020 Bull SAS
*****************************************************/

/****************************************************/
#include <gtest/gtest.h>
#include "../Container.hpp"
#include "../../backends/MemoryBackendMalloc.hpp"

/****************************************************/
using namespace IOC;

/****************************************************/
TEST(TestContainer, constructor)
{
	MemoryBackendMalloc mback(NULL);
	Container container(NULL, &mback);
}

/****************************************************/
TEST(TestContainer, getObject)
{
	MemoryBackendMalloc mback(NULL);
	Container container(NULL, &mback);
	Object & obj1 = container.getObject(ObjectId(10,20));
	Object & obj2 = container.getObject(ObjectId(10,21));
	Object & obj3 = container.getObject(ObjectId(10,20));

	EXPECT_NE(&obj1, &obj2);
	EXPECT_EQ(&obj1, &obj3);
}

/****************************************************/
TEST(TestContainer, hasObject)
{
	MemoryBackendMalloc mback(NULL);
	ObjectId objectId(10, 20);
	Container container(NULL, &mback);
	ASSERT_FALSE(container.hasObject(objectId));
	container.getObject(objectId);
	ASSERT_TRUE(container.hasObject(objectId));

}

/****************************************************/
TEST(TestContainer, onClientDisconnect)
{
	MemoryBackendMalloc mback(NULL);
	Container container(NULL, &mback);
	Object & obj1 = container.getObject(ObjectId(10,20));
	Object & obj2 = container.getObject(ObjectId(10,21));

	//client on obj1
	obj1.getConsistencyTracker().registerRange(0, 100, 100, CONSIST_ACCESS_MODE_WRITE);
	obj1.getConsistencyTracker().registerRange(1, 200, 100, CONSIST_ACCESS_MODE_WRITE);
	
	//client on obj2
	obj2.getConsistencyTracker().registerRange(0, 100, 100, CONSIST_ACCESS_MODE_WRITE);
	obj2.getConsistencyTracker().registerRange(1, 200, 100, CONSIST_ACCESS_MODE_WRITE);

	//check
	ASSERT_TRUE(obj1.getConsistencyTracker().hasCollision(100, 100, CONSIST_ACCESS_MODE_WRITE));
	ASSERT_TRUE(obj1.getConsistencyTracker().hasCollision(200, 100, CONSIST_ACCESS_MODE_WRITE));
	ASSERT_TRUE(obj2.getConsistencyTracker().hasCollision(100, 100, CONSIST_ACCESS_MODE_WRITE));
	ASSERT_TRUE(obj2.getConsistencyTracker().hasCollision(200, 100, CONSIST_ACCESS_MODE_WRITE));

	//disconnect
	container.onClientDisconnect(0);

	//check
	ASSERT_FALSE(obj1.getConsistencyTracker().hasCollision(100, 100, CONSIST_ACCESS_MODE_WRITE));
	ASSERT_TRUE(obj1.getConsistencyTracker().hasCollision(200, 100, CONSIST_ACCESS_MODE_WRITE));
	ASSERT_FALSE(obj2.getConsistencyTracker().hasCollision(100, 100, CONSIST_ACCESS_MODE_WRITE));
	ASSERT_TRUE(obj2.getConsistencyTracker().hasCollision(200, 100, CONSIST_ACCESS_MODE_READ));
}

/****************************************************/
TEST(TestContainer, makeObjectCow_ok_1)
{
	MemoryBackendMalloc mback(NULL);
	Container container(NULL, &mback);
	container.getObject(ObjectId(10,20));
	bool res = container.makeObjectFullCow(ObjectId(10,20), ObjectId(10,21), false);
	ASSERT_TRUE(res);
}

/****************************************************/
TEST(TestContainer, makeObjectCow_ok_2)
{
	MemoryBackendMalloc mback(NULL);
	Container container(NULL, &mback);
	container.getObject(ObjectId(10,20));
	container.getObject(ObjectId(10,21));
	bool res = container.makeObjectFullCow(ObjectId(10,20), ObjectId(10,21), true);
	ASSERT_TRUE(res);
}

/****************************************************/
TEST(TestContainer, makeObjectCow_alread_exist)
{
	MemoryBackendMalloc mback(NULL);
	Container container(NULL, &mback);
	container.getObject(ObjectId(10,20));
	container.getObject(ObjectId(10,21));
	bool res = container.makeObjectFullCow(ObjectId(10,20), ObjectId(10,21), false);
	ASSERT_FALSE(res);
}

/****************************************************/
TEST(TestContainer, makeObjectRangeCow_ok_1)
{
	MemoryBackendMalloc mback(NULL);
	Container container(NULL, &mback);
	Object & orig = container.getObject(ObjectId(10,20));
	orig.fillBuffer(1000, 500, 1);
	bool res = container.makeObjectRangeCow(ObjectId(10,20), ObjectId(10,21), false, 1000, 700);
	ASSERT_TRUE(res);

	//check
	Object & dest = container.getObject(ObjectId(10,21));
	ASSERT_TRUE(dest.checkUniq(1000, 500));
	ASSERT_TRUE(dest.checkUniq(1500, 200));
}

/****************************************************/
TEST(TestContainer, makeObjectRangeCow_ok_exist)
{
	MemoryBackendMalloc mback(NULL);
	Container container(NULL, &mback);
	container.getObject(ObjectId(10,20));
	container.getObject(ObjectId(10,21));
	bool res = container.makeObjectRangeCow(ObjectId(10,20), ObjectId(10,21), false, 1000, 500);
	ASSERT_FALSE(res);
}
