/*******************************************************************************
 *
 * Copyright (c) 2017 Tamás Seller. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *******************************************************************************/
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
