/*
 * TestConstantStringMatcher.cpp
 *
 *  Created on: 2017.02.09.
 *      Author: tooma
 */

#include "CppUTest/TestHarness.h"

#include "ConstantStringMatcher.h"

TEST_GROUP(ConstantStringMatcher)
{
	ConstantStringMatcher uut;
};

TEST(ConstantStringMatcher, Sanity)
{
	static constexpr const char* str = "foo";

	uut.reset();
	uut.progressWithMatching(str, "foo", 3);
	CHECK(uut.matches(str));
}

TEST(ConstantStringMatcher, ResetMatchAfterMatch)
{
	static constexpr const char* str = "foo";

	uut.reset();
	uut.progressWithMatching(str, "foo", 3);

	uut.reset();
	uut.progressWithMatching(str, "foo", 3);
	CHECK(uut.matches(str));
}

TEST(ConstantStringMatcher, ResetMatchAfterNonMatching)
{
	static constexpr const char* str = "foo";

	uut.reset();
	uut.progressWithMatching(str, "fo", 2);
	CHECK(!uut.matches(str));

	uut.reset();
	uut.progressWithMatching(str, "foo", 3);
	CHECK(uut.matches(str));
}


TEST(ConstantStringMatcher, ResetAfterNoMatchAfterMatching)
{
	static constexpr const char* str = "foo";

	uut.reset();
	uut.progressWithMatching(str, "foo", 3);
	CHECK(uut.matches(str));

	uut.reset();
	uut.progressWithMatching(str, "fo", 2);
	CHECK(!uut.matches(str));
}

TEST(ConstantStringMatcher, Segmented)
{
	static constexpr const char* str = "foo";
	uut.reset();
	uut.progressWithMatching(str, "f", 1);
	CHECK(!uut.matches(str));
	uut.progressWithMatching(str, "", 0);
	CHECK(!uut.matches(str));
	uut.progressWithMatching(str, "o", 1);
	CHECK(!uut.matches(str));
	uut.progressWithMatching(str, "o", 1);
	CHECK(uut.matches(str));
	uut.progressWithMatching(str, "x", 1);
}


TEST(ConstantStringMatcher, SegmentedFail)
{
	static constexpr const char* str = "foo";
	uut.reset();
	uut.progressWithMatching(str, "b", 1);
	CHECK(!uut.matches(str));
	uut.progressWithMatching(str, "a", 1);
	CHECK(!uut.matches(str));
	uut.progressWithMatching(str, "r", 1);
	CHECK(!uut.matches(str));
}
