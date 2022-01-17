/*****************************************************
*  PROJECT  : IO Catcher                             *
*  LICENSE  : Apache 2.0                             *
*  COPYRIGHT: 2020-2022 Bull SAS All rights reserved *
*****************************************************/

/****************************************************/
#include <gtest/gtest.h>
#include "../Object.hpp"
#include "../../backends/StorageBackendGMock.hpp"
#include <gmock/gmock.h>

/****************************************************/
using namespace IOC;
using namespace testing;

/****************************************************/
/**
 * Implement a storage backend for the Mero object storage.
**/
class StorageBackendGMockNoCow : public StorageBackendGMock
{
	public:
		virtual ssize_t makeCowSegment(int64_t highOrig, int64_t lowOrig, int64_t highDest, int64_t lowDest, size_t offset, size_t size) {
			return StorageBackend::makeCowSegment(highOrig, lowOrig, highDest, lowDest, offset, size);
		};
};

/****************************************************/
TEST(TestBackend, makeCowSegment)
{
	StorageBackendGMockNoCow storage;

	//expect call to write
	EXPECT_CALL(storage, pread(20, 10, _, 500, 1000))
		.Times(1)
		.WillOnce(Return(500));
		//expect call to write
	EXPECT_CALL(storage, pwrite(20, 11, _, 500, 1000))
		.Times(1)
		.WillOnce(Return(500));

	storage.makeCowSegment(20, 10, 20, 11, 1000, 500);
}
