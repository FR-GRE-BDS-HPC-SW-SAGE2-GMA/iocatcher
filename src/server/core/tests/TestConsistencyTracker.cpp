/*****************************************************
            PROJECT  : IO Catcher
            LICENSE  : Apache 2.0
			COPYRIGHT: 2020 Bull SAS
COPYRIGHT: 2020 Bull SAS
*****************************************************/

/****************************************************/
#include <gtest/gtest.h>
#include "../ConsistencyTracker.hpp"

/****************************************************/
using namespace IOC;

/****************************************************/
TEST(TestConsistencyTracker, overlap)
{
	EXPECT_TRUE(ConsistencyTracker::overlap(20,10,20,10));
	EXPECT_TRUE(ConsistencyTracker::overlap(20,10,00,30));
	EXPECT_TRUE(ConsistencyTracker::overlap(20,10,25,10));

	EXPECT_FALSE(ConsistencyTracker::overlap(20,10,0,20));
	EXPECT_FALSE(ConsistencyTracker::overlap(20,10,30,10));
}

/****************************************************/
TEST(TestConsistencyTracker, constructor)
{
	ConsistencyTracker tracker;
}

/****************************************************/
TEST(TestConsistencyTracker, register_unregister)
{
	uint64_t clientId = 0;
	ConsistencyTracker tracker;
	ssize_t id = tracker.registerRange(clientId, 200, 100, CONSIST_ACCESS_MODE_WRITE);
	ASSERT_GE(id, 0);
	ASSERT_TRUE(tracker.hasCollision(200, 10, CONSIST_ACCESS_MODE_WRITE));

	ASSERT_TRUE(tracker.unregisterRange(clientId, id, 200, 100, CONSIST_ACCESS_MODE_WRITE));
	ASSERT_FALSE(tracker.unregisterRange(clientId, id, 200, 100, CONSIST_ACCESS_MODE_WRITE));
	ASSERT_FALSE(tracker.hasCollision(200, 10, CONSIST_ACCESS_MODE_WRITE));
}

/****************************************************/
TEST(TestConsistencyTracker, register_unregister_invalid)
{
	uint64_t clientId = 0;
	ConsistencyTracker tracker;
	ssize_t id = tracker.registerRange(clientId, 200, 100, CONSIST_ACCESS_MODE_WRITE);
	ASSERT_GE(id, 0);
	ASSERT_TRUE(tracker.hasCollision(200, 10, CONSIST_ACCESS_MODE_WRITE));

	//invalid id
	ASSERT_FALSE(tracker.unregisterRange(clientId, 100, 200, 100, CONSIST_ACCESS_MODE_WRITE));
	
	//invalid offset
	ASSERT_FALSE(tracker.unregisterRange(clientId, id, 100, 100, CONSIST_ACCESS_MODE_WRITE));
	
	//invalid mode
	ASSERT_FALSE(tracker.unregisterRange(clientId, id, 200, 100, CONSIST_ACCESS_MODE_READ));
	
	//invalid size
	ASSERT_FALSE(tracker.unregisterRange(clientId, id, 200, 300, CONSIST_ACCESS_MODE_READ));
}

/****************************************************/
TEST(TestConsistencyTracker, alloc_multi_read)
{
	uint64_t clientId = 0;
	ConsistencyTracker tracker;
	ASSERT_EQ(1, tracker.registerRange(clientId, 200, 100, CONSIST_ACCESS_MODE_READ));
	ASSERT_EQ(2, tracker.registerRange(clientId, 200, 10, CONSIST_ACCESS_MODE_READ));
	ASSERT_EQ(3, tracker.registerRange(clientId, 210, 10, CONSIST_ACCESS_MODE_READ));
	ASSERT_EQ(4, tracker.registerRange(clientId, 200, 100, CONSIST_ACCESS_MODE_READ));
}

/****************************************************/
TEST(TestConsistencyTracker, uniq_write)
{
	uint64_t clientId = 0;
	ConsistencyTracker tracker;
	ASSERT_EQ(1, tracker.registerRange(clientId, 200, 100, CONSIST_ACCESS_MODE_WRITE));
	ASSERT_EQ(-1, tracker.registerRange(clientId, 200, 100, CONSIST_ACCESS_MODE_WRITE));
}

/****************************************************/
TEST(TestConsistencyTracker, multi_read_no_write)
{
	uint64_t clientId = 0;
	ConsistencyTracker tracker;
	ASSERT_EQ(1, tracker.registerRange(clientId, 200, 100, CONSIST_ACCESS_MODE_READ));
	ASSERT_EQ(2, tracker.registerRange(clientId, 200, 100, CONSIST_ACCESS_MODE_READ));
	ASSERT_EQ(-1, tracker.registerRange(clientId, 200, 100, CONSIST_ACCESS_MODE_WRITE));
}


/****************************************************/
TEST(TestConsistencyTracker, multi_overlap)
{
	uint64_t clientId = 0;
	ConsistencyTracker tracker;
	ASSERT_EQ(1, tracker.registerRange(clientId, 200, 100, CONSIST_ACCESS_MODE_READ));
	ASSERT_EQ(2, tracker.registerRange(clientId, 300, 100, CONSIST_ACCESS_MODE_WRITE));
	ASSERT_EQ(-1, tracker.registerRange(clientId, 200, 200, CONSIST_ACCESS_MODE_READ));
}

/****************************************************/
TEST(TestConsistencyTracker, clientDisconnect)
{
	//setup
	uint64_t clientId = 0;
	ConsistencyTracker tracker;

	//register
	ASSERT_EQ(1, tracker.registerRange(clientId, 200, 100, CONSIST_ACCESS_MODE_READ));
	ASSERT_EQ(2, tracker.registerRange(clientId, 300, 100, CONSIST_ACCESS_MODE_WRITE));
	ASSERT_EQ(3, tracker.registerRange(clientId+1, 400, 100, CONSIST_ACCESS_MODE_WRITE));
	
	//check
	ASSERT_TRUE(tracker.hasCollision(200, 10, CONSIST_ACCESS_MODE_WRITE));
	ASSERT_TRUE(tracker.hasCollision(300, 10, CONSIST_ACCESS_MODE_WRITE));
	ASSERT_TRUE(tracker.hasCollision(400, 10, CONSIST_ACCESS_MODE_WRITE));

	//close client
	tracker.clientDisconnect(clientId);

	//check
	ASSERT_FALSE(tracker.hasCollision(200, 10, CONSIST_ACCESS_MODE_WRITE));
	ASSERT_FALSE(tracker.hasCollision(300, 10, CONSIST_ACCESS_MODE_WRITE));
	ASSERT_TRUE(tracker.hasCollision(400, 10, CONSIST_ACCESS_MODE_WRITE));
}
