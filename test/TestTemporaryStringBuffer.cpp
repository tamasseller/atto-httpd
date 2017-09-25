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

#include "1test/Test.h"

#include "TemporaryStringBuffer.h"

TEST_GROUP(TemporaryStringBuffer) {
	TemporaryStringBuffer<8> uut;

};

TEST(TemporaryStringBuffer, Cleared)
{
	uut.terminate();
	CHECK(strlen(uut.data()) == 0);
	CHECK(uut.length() == strlen(uut.data()));
}

TEST(TemporaryStringBuffer, Sanity)
{
	uut.clear();
	uut.terminate();
	CHECK(strlen(uut.data()) == 0);
}

TEST(TemporaryStringBuffer, Short)
{
	CHECK(uut.save("foo", 3));
	uut.terminate();
	CHECK(strlen(uut.data()) == 3);
	CHECK(strcmp(uut.data(), "foo") == 0);

	uut.clear();
	uut.terminate();
	CHECK(strlen(uut.data()) == 0);
}

TEST(TemporaryStringBuffer, Clear)
{
	CHECK(uut.save("foo", 3));
	uut.terminate();
	CHECK(uut.length() == strlen(uut.data()));
	uut.clear();
	uut.terminate();
	CHECK(uut.length() == strlen(uut.data()));
	CHECK(strlen(uut.data()) == 0);
}


TEST(TemporaryStringBuffer, Concat)
{
	CHECK(uut.save("foo", 3));
	CHECK(uut.save("bar", 3));
	uut.terminate();
	CHECK(uut.length() == strlen(uut.data()));
	CHECK(strlen(uut.data()) == 6);
	CHECK(strcmp(uut.data(), "foobar") == 0);
}

TEST(TemporaryStringBuffer, TooLong)
{
	CHECK(uut.save("bad", 3));
	CHECK(!uut.save("strings", 7));
	uut.terminate();
	CHECK(uut.length() == strlen(uut.data()));
	CHECK(strlen(uut.data()) == 7);
	CHECK(strcmp(uut.data(), "badstri") == 0);
}

TEST(TemporaryStringBuffer, OverloadThenClear)
{
	CHECK(!uut.save("overloaded", 10));
	uut.clear();
	uut.terminate();
	CHECK(uut.length() == strlen(uut.data()));
	CHECK(strlen(uut.data()) == 0);
}
