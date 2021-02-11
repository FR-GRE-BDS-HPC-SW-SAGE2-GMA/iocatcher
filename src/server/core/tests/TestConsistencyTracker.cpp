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
	ConsistencyTracker tracker;
	ssize_t id = tracker.registerRange(200, 100, CONSIST_ACCESS_MODE_WRITE);
	ASSERT_GE(id, 0);
	ASSERT_TRUE(tracker.hasCollision(200, 10, CONSIST_ACCESS_MODE_WRITE));

	ASSERT_TRUE(tracker.unregisterRange(id, 200, 100, CONSIST_ACCESS_MODE_WRITE));
	ASSERT_FALSE(tracker.unregisterRange(id, 200, 100, CONSIST_ACCESS_MODE_WRITE));
	ASSERT_FALSE(tracker.hasCollision(200, 10, CONSIST_ACCESS_MODE_WRITE));
}

/****************************************************/
TEST(TestConsistencyTracker, register_unregister_invalid)
{
	ConsistencyTracker tracker;
	ssize_t id = tracker.registerRange(200, 100, CONSIST_ACCESS_MODE_WRITE);
	ASSERT_GE(id, 0);
	ASSERT_TRUE(tracker.hasCollision(200, 10, CONSIST_ACCESS_MODE_WRITE));

	//invalid id
	ASSERT_FALSE(tracker.unregisterRange(100, 200, 100, CONSIST_ACCESS_MODE_WRITE));
	
	//invalid offset
	ASSERT_FALSE(tracker.unregisterRange(id, 100, 100, CONSIST_ACCESS_MODE_WRITE));
	
	//invalid mode
	ASSERT_FALSE(tracker.unregisterRange(id, 200, 100, CONSIST_ACCESS_MODE_READ));
	
	//invalid size
	ASSERT_FALSE(tracker.unregisterRange(id, 200, 300, CONSIST_ACCESS_MODE_READ));
}

/****************************************************/
TEST(TestConsistencyTracker, alloc_multi_read)
{
	ConsistencyTracker tracker;
	ASSERT_EQ(1, tracker.registerRange(200, 100, CONSIST_ACCESS_MODE_READ));
	ASSERT_EQ(2, tracker.registerRange(200, 10, CONSIST_ACCESS_MODE_READ));
	ASSERT_EQ(3, tracker.registerRange(210, 10, CONSIST_ACCESS_MODE_READ));
	ASSERT_EQ(4, tracker.registerRange(200, 100, CONSIST_ACCESS_MODE_READ));
}

/****************************************************/
TEST(TestConsistencyTracker, uniq_write)
{
	ConsistencyTracker tracker;
	ASSERT_EQ(1, tracker.registerRange(200, 100, CONSIST_ACCESS_MODE_WRITE));
	ASSERT_EQ(-1, tracker.registerRange(200, 100, CONSIST_ACCESS_MODE_WRITE));
}

/****************************************************/
TEST(TestConsistencyTracker, multi_read_no_write)
{
	ConsistencyTracker tracker;
	ASSERT_EQ(1, tracker.registerRange(200, 100, CONSIST_ACCESS_MODE_READ));
	ASSERT_EQ(2, tracker.registerRange(200, 100, CONSIST_ACCESS_MODE_READ));
	ASSERT_EQ(-1, tracker.registerRange(200, 100, CONSIST_ACCESS_MODE_WRITE));
}


/****************************************************/
TEST(TestConsistencyTracker, multi_overlap)
{
	ConsistencyTracker tracker;
	ASSERT_EQ(1, tracker.registerRange(200, 100, CONSIST_ACCESS_MODE_READ));
	ASSERT_EQ(2, tracker.registerRange(300, 100, CONSIST_ACCESS_MODE_WRITE));
	ASSERT_EQ(-1, tracker.registerRange(200, 200, CONSIST_ACCESS_MODE_READ));
}
