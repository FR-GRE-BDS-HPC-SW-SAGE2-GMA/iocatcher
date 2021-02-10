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
	ASSERT_TRUE(tracker.registerRange(200, 100, CONSIST_ACCESS_MODE_WRITE));
	ASSERT_TRUE(tracker.hasCollision(200, 10, CONSIST_ACCESS_MODE_WRITE));

	ASSERT_TRUE(tracker.unregisterRange(200, 100));
	ASSERT_FALSE(tracker.unregisterRange(200, 100));
	ASSERT_FALSE(tracker.hasCollision(200, 10, CONSIST_ACCESS_MODE_WRITE));
}

/****************************************************/
TEST(TestConsistencyTracker, alloc_multi_read)
{
	ConsistencyTracker tracker;
	ASSERT_TRUE(tracker.registerRange(200, 100, CONSIST_ACCESS_MODE_READ));
	ASSERT_TRUE(tracker.registerRange(200, 10, CONSIST_ACCESS_MODE_READ));
	ASSERT_TRUE(tracker.registerRange(210, 10, CONSIST_ACCESS_MODE_READ));
	ASSERT_TRUE(tracker.registerRange(200, 100, CONSIST_ACCESS_MODE_READ));
}

/****************************************************/
TEST(TestConsistencyTracker, uniq_write)
{
	ConsistencyTracker tracker;
	ASSERT_TRUE(tracker.registerRange(200, 100, CONSIST_ACCESS_MODE_WRITE));
	ASSERT_FALSE(tracker.registerRange(200, 100, CONSIST_ACCESS_MODE_WRITE));
}

/****************************************************/
TEST(TestConsistencyTracker, multi_read_no_write)
{
	ConsistencyTracker tracker;
	ASSERT_TRUE(tracker.registerRange(200, 100, CONSIST_ACCESS_MODE_READ));
	ASSERT_TRUE(tracker.registerRange(200, 100, CONSIST_ACCESS_MODE_READ));
	ASSERT_FALSE(tracker.registerRange(200, 100, CONSIST_ACCESS_MODE_WRITE));
}


/****************************************************/
TEST(TestConsistencyTracker, multi_overlap)
{
	ConsistencyTracker tracker;
	ASSERT_TRUE(tracker.registerRange(200, 100, CONSIST_ACCESS_MODE_READ));
	ASSERT_TRUE(tracker.registerRange(300, 100, CONSIST_ACCESS_MODE_WRITE));
	ASSERT_FALSE(tracker.registerRange(200, 200, CONSIST_ACCESS_MODE_READ));
}
