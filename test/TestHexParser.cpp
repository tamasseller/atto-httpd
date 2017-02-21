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

#include "HexParser.h"

#include <string.h>

TEST_GROUP(HexParser) {
	HexParser<4> uut;
};

TEST(HexParser, Sanity) {
	const char *input = "600dc0de";
	const unsigned char expected[] = {0x60, 0x0d, 0xc0, 0xde};

	uut.clear();
	CHECK(!uut.isDone());

	uut.parseHex(input, strlen(input));

	CHECK(uut.isDone());
	CHECK(memcmp(uut.data(), expected, sizeof(expected)) == 0);
}

TEST(HexParser, Case) {
	const char *input = "b16C0DE5";
	const unsigned char expected[] = {0xb1, 0x6c, 0x0d, 0xe5};

	uut.clear();
	uut.parseHex(input, strlen(input));

	CHECK(uut.isDone());
	CHECK(memcmp(uut.data(), expected, sizeof(expected)) == 0);
}

TEST(HexParser, Bytes) {
	const char *input1 = "50";
	const char *input2 = "33d4";
	const char *input3 = "74";
	const unsigned char expected[] = {0x50, 0x33, 0xd4, 0x74};

	uut.clear();

	uut.parseHex(input1, strlen(input1));
	CHECK(!uut.isDone());

	uut.parseHex(input2, strlen(input2));
	CHECK(!uut.isDone());

	uut.parseHex(input3, strlen(input3));
	CHECK(uut.isDone());

	CHECK(memcmp(uut.data(), expected, sizeof(expected)) == 0);
}

TEST(HexParser, Halves) {
	const char *input1 = "8a7";
	const char *input2 = "43";
	const char *input3 = "d6d";
	const unsigned char expected[] = {0x8a, 0x74, 0x3d, 0x6d};

	uut.clear();

	uut.parseHex(input1, strlen(input1));
	CHECK(!uut.isDone());

	uut.parseHex(input2, strlen(input2));
	CHECK(!uut.isDone());

	uut.parseHex(input3, strlen(input3));
	CHECK(uut.isDone());

	CHECK(memcmp(uut.data(), expected, sizeof(expected)) == 0);
}

TEST(HexParser, Reset) {
	const char *input1 = "0020";
	const char *input2 = "0001";
	const unsigned char expected[] = {0x00, 0x01, 0x00, 0x20};

	uut.clear();

	uut.parseHex(input1, strlen(input1));
	uut.parseHex(input2, strlen(input2));

	uut.clear();
	CHECK(!uut.isDone());

	uut.parseHex(input2, strlen(input2));
	CHECK(!uut.isDone());

	uut.parseHex(input1, strlen(input1));
	CHECK(uut.isDone());

	CHECK(memcmp(uut.data(), expected, sizeof(expected)) == 0);
}


TEST(HexParser, Overload) {
	const char *input1 = "0020";
	const char *input2 = "0001";

	uut.clear();

	uut.parseHex(input1, strlen(input1));
	uut.parseHex(input2, strlen(input2));
	uut.parseHex(input1, strlen(input1));
	uut.parseHex(input2, strlen(input2));

	CHECK(!uut.isDone());
}

TEST(HexParser, Bullshit) {
	const char *input1 = "bull";
	const char *input2 = "shit";
	const char *input3 = "\n";
	const char *input4 = "\xff";

	uut.clear();
	uut.parseHex(input1, strlen(input1));

	uut.clear();
	uut.parseHex(input2, strlen(input2));

	uut.clear();
	uut.parseHex(input4, strlen(input3));

	uut.clear();
	uut.parseHex(input3, strlen(input4));

	CHECK(!uut.isDone());
}
