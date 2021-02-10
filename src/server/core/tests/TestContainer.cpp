/*****************************************************
            PROJECT  : IO Catcher
            LICENSE  : Apache 2.0
			COPYRIGHT: 2020 Bull SAS
COPYRIGHT: 2020 Bull SAS
*****************************************************/

/****************************************************/
#include <gtest/gtest.h>
#include "../Container.hpp"

/****************************************************/
using namespace IOC;

/****************************************************/
TEST(TestContainer, constructor)
{
	Container container(NULL);
}

/****************************************************/
TEST(TestContainer, getObject)
{
	Container container(NULL);
	Object & obj1 = container.getObject(10,20);
	Object & obj2 = container.getObject(10,21);
	Object & obj3 = container.getObject(10,20);

	EXPECT_NE(&obj1, &obj2);
	EXPECT_EQ(&obj1, &obj3);
}
