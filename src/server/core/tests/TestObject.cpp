/*****************************************************
            PROJECT  : IO Catcher
            LICENSE  : Apache 2.0
			COPYRIGHT: 2020 Bull SAS
COPYRIGHT: 2020 Bull SAS
*****************************************************/

/****************************************************/
#include <gtest/gtest.h>
#include "../Object.hpp"

/****************************************************/
using namespace IOC;

/****************************************************/
TEST(TestObjectSegment, overlap)
{
	//build
	ObjectSegment seg;
	seg.offset = 1000;
	seg.size = 500;

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
	Object object(NULL, 10, 10);

	ObjectSegmentList lst;
	object.getBuffers(lst, 1000,500);
	EXPECT_EQ(1, lst.size());
}

/****************************************************/
TEST(TestObject, getBuffers_2)
{
	Object object(NULL, 10, 10);

	ObjectSegmentList lst;
	object.getBuffers(lst, 1000,500);
	EXPECT_EQ(1, lst.size());

	lst.clear();
	object.getBuffers(lst, 1000,1000);
	EXPECT_EQ(2, lst.size());
}

/****************************************************/
TEST(TestObject, getBuffers_3)
{
	Object object(NULL, 10, 10);

	ObjectSegmentList lst;
	object.getBuffers(lst, 1000,300);
	EXPECT_EQ(1, lst.size());

	lst.clear();
	object.getBuffers(lst, 1600,300);
	EXPECT_EQ(1, lst.size());

	lst.clear();
	object.getBuffers(lst, 1000,900);
	EXPECT_EQ(3, lst.size());
}

/****************************************************/
TEST(TestObject, getBuffers_4)
{
	Object object(NULL, 10, 10);

	ObjectSegmentList lst;
	object.getBuffers(lst, 1000,500);
	EXPECT_EQ(1, lst.size());

	lst.clear();
	object.getBuffers(lst, 800,1800);
	EXPECT_EQ(3, lst.size());
}

/****************************************************/
TEST(TestObject, getBuffers_5_alignement)
{
	Object object(NULL, 10, 10, 1000);

	ObjectSegmentList lst;
	object.getBuffers(lst, 1000,500);
	EXPECT_EQ(1, lst.size());

	lst.clear();
	object.getBuffers(lst, 1000,1000);
	EXPECT_EQ(1, lst.size());
}

/****************************************************/
TEST(TestObject, getBuffers_6_alignement)
{
	Object object(NULL, 10, 10, 1000);

	ObjectSegmentList lst;
	object.getBuffers(lst, 1000,500);
	EXPECT_EQ(1, lst.size());

	lst.clear();
	object.getBuffers(lst, 1000,1500);
	EXPECT_EQ(2, lst.size());
}
