/*
 * TestTemporaryStringBuffer.h
 *
 *  Created on: 2017.02.09.
 *      Author: tooma
 */

#include "CppUTest/TestHarness.h"

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
