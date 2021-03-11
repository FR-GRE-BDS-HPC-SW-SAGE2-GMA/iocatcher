/*****************************************************
			PROJECT  : IO Catcher
			LICENSE  : Apache 2.0
			COPYRIGHT: 2020 Bull SAS
*****************************************************/

/****************************************************/
#include <gtest/gtest.h>
#include "../ObjectSegment.hpp"

/****************************************************/
using namespace IOC;
using namespace testing;

/****************************************************/
TEST(TestObjectSegment, default_constructor)
{
	ObjectSegment build;
}

/****************************************************/
TEST(TestObjectSegment, value_constructor)
{
	//build orig
	void * buffer = malloc(64);
	ObjectSegment segment(512, 64, (char*)buffer, false);

	//test
	EXPECT_EQ(64, segment.getSize());
	EXPECT_EQ(512, segment.getOffset());
	EXPECT_FALSE(segment.isDirty());
	EXPECT_FALSE(segment.isCow());
	EXPECT_EQ(buffer, (void*)segment.getBuffer());
}

/****************************************************/
TEST(TestObjectSegment, move_constructor)
{
	//build orig
	void * buffer = malloc(64);
	ObjectSegment orig(512, 64, (char*)buffer, false);

	//make move
	ObjectSegment second(std::move(orig));

	//test
	EXPECT_EQ(64, second.getSize());
	EXPECT_EQ(512, second.getOffset());
	EXPECT_FALSE(second.isDirty());
	EXPECT_FALSE(second.isCow());
	EXPECT_EQ(buffer, (void*)second.getBuffer());
}

/****************************************************/
TEST(TestObjectSegment, overlap)
{
	//build
	size_t offset = 1000;
	size_t size = 500;
	ObjectSegment segment(offset, size, NULL, false);

	//check cases
	EXPECT_TRUE(segment.overlap(1000,500));
	EXPECT_TRUE(segment.overlap(800,500));
	EXPECT_TRUE(segment.overlap(1200,500));
	EXPECT_FALSE(segment.overlap(1500,500));
	EXPECT_FALSE(segment.overlap(500,500));
}

/****************************************************/
TEST(TestObjectSegment, getSegmentDescr)
{
	//build orig
	void * buffer = malloc(64);
	ObjectSegment segment(512, 64, (char*)buffer, false);

	//get descr
	ObjectSegmentDescr descr = segment.getSegmentDescr();

	//check
	EXPECT_EQ(buffer, (void*)descr.ptr);
	EXPECT_EQ(64, descr.size);
	EXPECT_EQ(512, descr.offset);
}

/****************************************************/
TEST(TestObjectSegment, get_set_params)
{
	//build orig
	ObjectSegment segment(512, 64, NULL, false);

	//get descr
	EXPECT_EQ(64, segment.getSize());
	EXPECT_EQ(512, segment.getOffset());
	EXPECT_EQ(nullptr, (void*)segment.getBuffer());
	EXPECT_FALSE(segment.isDirty());

	//set dirty
	segment.setDirty(true);
	EXPECT_TRUE(segment.isDirty());
}

/****************************************************/
TEST(TestObjectSegment, cow_support)
{
	//build orig
	void * buffer = malloc(64);
	ObjectSegment orig(512, 64, (char*)buffer, false);
	orig.setDirty(true);
	ASSERT_EQ(buffer, (void*)orig.getBuffer());

	//make cow
	ObjectSegment cow;
	cow.makeCowOf(orig);

	//check orig
	ASSERT_EQ(buffer, (void*)orig.getBuffer());
	ASSERT_TRUE(orig.isCow());

	//check cow
	ASSERT_EQ(buffer, (void*)cow.getBuffer());
	ASSERT_EQ(64, cow.getSize());
	ASSERT_EQ(512, cow.getOffset());
	ASSERT_TRUE(cow.isDirty());
	ASSERT_TRUE(cow.isCow());

	//apply cow
	void * new_buffer = malloc(64);
	cow.applyCow((char*)new_buffer, 64, false);

	//check orig
	ASSERT_EQ(buffer, (void*)orig.getBuffer());
	ASSERT_FALSE(orig.isCow());

	//check cow
	ASSERT_EQ(new_buffer, (void*)cow.getBuffer());
	ASSERT_FALSE(cow.isCow());
}

/****************************************************/
TEST(TestObjectSegment, move_operator)
{
	//build orig
	void * buffer = malloc(64);
	ObjectSegment orig(512, 64, (char*)buffer, false);

	//make move
	ObjectSegment second = std::move(orig);

	//test
	EXPECT_EQ(64, second.getSize());
	EXPECT_EQ(512, second.getOffset());
	EXPECT_FALSE(second.isDirty());
	EXPECT_FALSE(second.isCow());
	EXPECT_EQ(buffer, (void*)second.getBuffer());
}
