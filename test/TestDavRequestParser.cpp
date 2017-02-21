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

#include "DavRequestParser.h"

#include <string.h>

#include <list>
#include <string>

TEST_GROUP(DavReq) {
	typedef DavRequestParser<192> Uut;
	Uut uut;
};

TEST(DavReq, Allprop) {
	constexpr const char* input =
			"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
			"<propfind xmlns=\"DAV:\"><allprop/></propfind>";

	for(unsigned int i = 0; i<strlen(input); i++) {
		uut.reset();
		CHECK(uut.parseDavRequest(input, i));
		CHECK(uut.parseDavRequest(input + i, strlen(input)-i));
		CHECK(uut.done());
		CHECK(uut.getType() == Uut::Type::Allprop);
		auto it = uut.propertyIterator();
		CHECK(!it.isValid());
		CHECK(!uut.step(it));
		CHECK(!it.isValid());
	}
}

TEST(DavReq, Propname) {
	constexpr const char* input =
			"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
			"<propfind xmlns=\"DAV:\"><propname/></propfind>";

	for(unsigned int i = 0; i<strlen(input); i++) {
		uut.reset();
		CHECK(uut.parseDavRequest(input, i));
		CHECK(uut.parseDavRequest(input + i, strlen(input)-i));
		CHECK(uut.done());
		CHECK(uut.getType() == Uut::Type::Propname);
		CHECK(!uut.propertyIterator().isValid());
	}
}

TEST(DavReq, Prop) {
	constexpr const char* input =
			"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
			"<D:propfind xmlns:D=\"DAV:\">\n"
			"\t<prop>\n"
			"\t\t<getcontentlength xmlns=\"DAV:\"/>\n"
			"\t\t<getlastmodified xmlns=\"DAV:\"/>\n"
			"\t\t<executable xmlns=\"http://apache.org/dav/props/\"/>\n"
			"\t\t<resourcetype xmlns=\"DAV:\"/>\n"
			"\t\t<checked-in xmlns=\"DAV:\"/>\n"
			"\t\t<checked-out xmlns=\"DAV:\"/>\n"
			"\t</prop>\n"
			"</D:propfind>\n";

	std::list<std::pair<std::string, std::string>> expected {
		{"checked-out", "DAV:"},
		{"checked-in", "DAV:"},
		{"resourcetype", "DAV:"},
		{"executable", "http://apache.org/dav/props/"},
		{"getlastmodified", "DAV:"},
		{"getcontentlength", "DAV:"}
	};

	for(unsigned int i = 0; i<strlen(input); i++) {
		uut.reset();
		CHECK(uut.parseDavRequest(input, i));
		CHECK(uut.parseDavRequest(input + i, strlen(input)-i));
		CHECK(uut.done());
		CHECK(uut.getType() == Uut::Type::Prop);

		auto expIt = expected.begin();
		for(auto it = uut.propertyIterator(); it.isValid(); uut.step(it)) {
			const char *str;
			uint32_t len;
			it.getName(str, len);
			CHECK((*expIt).first == std::string(str, len));
			it.getNs(str, len);
			CHECK((*expIt).second == std::string(str, len));
			expIt++;
		}

		CHECK(expIt == expected.end());
	}
}

TEST(DavReq, TooLongProp) {
	constexpr const char* input =
			"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
			"<propfind xmlns=\"DAV:\">\n"
			"\t<prop>\n"
			"\t\t<extremelyveryverylongpropertyname />\n"
			"\t</prop>\n"
			"</propfind>\n";

	uut.reset();
	CHECK(!uut.parseDavRequest(input, strlen(input)));
	CHECK(!uut.done());
}

TEST(DavReq, TooLongToSaveNs) {
	constexpr const char* input =
			"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
			"<propfind xmlns=\"DAV:\">\n"
			"\t<prop>\n"
			"\t\t<severalProperties xmlns='with needlessly long namespace url'/>\n"
			"\t\t<thatWill xmlns='with needlessly long namespace url'/>\n"
			"\t\t<overload xmlns='with needlessly long namespace url'/>\n"
			"\t\t<theHeap xmlns='with needlessly long namespace url'/>\n"
			"\t</prop>\n"
			"</propfind>\n";

	uut.reset();
	CHECK(!uut.parseDavRequest(input, strlen(input)));
	CHECK(!uut.done());
}

TEST(DavReq, TooManyToParseNs) {
	constexpr const char* input =
			"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
			"<propfind "
			"xmlns=\"DAV:\""
			"xmlns:BS1=\"the unnecessary bloating of the xml\""
			"xmlns:BS2=\"with unused namespaces can easily \""
			"xmlns:BS3=\"use up parser stack space and thus\""
			"xmlns:BS4=\"screw up the whole request parsing operation\""
			"xmlns:BS5=\"for no reason \""
			"><allprop/></propfind>";

	uut.reset();
	CHECK(!uut.parseDavRequest(input, strlen(input) - 10));
	CHECK(!uut.parseDavRequest(input + strlen(input) - 10, 10));
	CHECK(!uut.done());
}

TEST(DavReq, Funny) {
	constexpr const char* truncated = "<propfind xmlns=\"DAV:\"><allprop/>";

	uut.reset();
	CHECK(uut.parseDavRequest(truncated, strlen(truncated)));
	CHECK(!uut.done());

	constexpr const char* ignoredAttributes = "<propfind xmlns=\"DAV:\"><allprop ignore='this'/></propfind>";

	uut.reset();
	CHECK(uut.parseDavRequest(ignoredAttributes, strlen(ignoredAttributes)));
	CHECK(uut.done());

	constexpr const char* garbage1 = "<something/>";

	uut.reset();
	CHECK(!uut.parseDavRequest(garbage1, strlen(garbage1)));
	CHECK(!uut.done());

	constexpr const char* garbage2 = "<propfind><something/></propfind>";

	uut.reset();
	CHECK(!uut.parseDavRequest(garbage2, strlen(garbage2)));
	CHECK(!uut.done());
}
