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
#include "CppUTestExt/MockSupport.h"

#include "UrlParser.h"

#include <string.h>

TEST_GROUP(UrlParser) {

	struct Dummy: public UrlParser<Dummy> {
		 void onQuery(const char* at, uint32_t length) {}
		 void queryDone() {}
		 void onPath(const char* at, uint32_t length) {}
		 void pathDone() {}
	};

	class Uut: public UrlParser<Uut> {
		friend UrlParser<Uut>;

		void onQuery(const char* at, uint32_t length) {
			strncat(query, at, length);
		}

		void queryDone() {
			CHECK(!qdone);
			qdone = true;
		}

		void onPath(const char* at, uint32_t length) {
			strncat(path, at, length);
		}

		void pathDone() {
			CHECK(!done);
			done = true;
		}

	public:
		char path[256], query[256];
		bool done, qdone;

		void reset() {
			UrlParser<Uut>::reset();
			done = false;
			qdone = false;
			bzero(path, sizeof(path));
			bzero(query, sizeof(path));
		}
	};

	Uut uut;
};


TEST(UrlParser, Dummy) {
	const char* url = "/foo?bar";
	Dummy dummy;
	dummy.reset();
	dummy.parseUrl(url, strlen(url));
	dummy.done();
}

TEST(UrlParser, Invalid) {
	uut.reset();
	const char* badUrl = " /foo \r\n\t\f";
	uut.parseUrl(badUrl, strlen(badUrl));
	uut.UrlParser<Uut>::done();
	CHECK(!uut.done);
	CHECK(!uut.path[0]);
	CHECK(!uut.qdone);
	CHECK(!uut.query[0]);
}

TEST(UrlParser, Invalid2) {
	uut.reset();
	const char* badUrl = "somep://foo@@bar";
	uut.parseUrl(badUrl, strlen(badUrl));
	uut.UrlParser<Uut>::done();
	CHECK(!uut.done);
	CHECK(!uut.path[0]);
	CHECK(!uut.qdone);
	CHECK(!uut.query[0]);
}


TEST(UrlParser, Sanity) {
	const char* url = "/foo";
	uut.reset();
	uut.parseUrl(url, strlen(url));
	uut.UrlParser<Uut>::done();
	CHECK(uut.done);
	CHECK(strcmp(uut.path, "/foo") == 0);
	CHECK(!uut.qdone);
	CHECK(!uut.query[0]);
}

TEST(UrlParser, Fragment) {
	const char* url = "/foo#bar";
	uut.reset();
	uut.parseUrl(url, strlen(url));
	uut.UrlParser<Uut>::done();
	CHECK(uut.done);
	CHECK(strcmp(uut.path, "/foo") == 0);
	CHECK(!uut.qdone);
	CHECK(!uut.query[0]);
}

TEST(UrlParser, At) {
	const char* url = "rfc6214://johny@foobar/stuffwarez";
	uut.reset();
	uut.parseUrl(url, strlen(url));
	uut.UrlParser<Uut>::done();
	CHECK(uut.done);
	CHECK(strcmp(uut.path, "/stuffwarez") == 0);
	CHECK(!uut.qdone);
	CHECK(!uut.query[0]);
}

TEST(UrlParser, QueryAndFragment) {
	const char* url = "/foo?bar#baz";
	uut.reset();
	uut.parseUrl(url, strlen(url));
	uut.UrlParser<Uut>::done();
	CHECK(uut.done);
	CHECK(strcmp(uut.path, "/foo") == 0);
	CHECK(uut.qdone);
	CHECK(strcmp(uut.query, "bar") == 0);
}

TEST(UrlParser, Stupid) {
	const char* url = "thing://foo?bar";
	uut.reset();
	uut.parseUrl(url, strlen(url));
	uut.UrlParser<Uut>::done();
	CHECK(!uut.done);
	CHECK(!uut.path[0]);
	CHECK(uut.qdone);
	CHECK(strcmp(uut.query, "bar") == 0);
}

TEST(UrlParser, Xmass) {
	const char* url = "scheme://prefix.domain:1234/path/filename?param1=7&param2=seven.##junk";
	uut.reset();
	uut.parseUrl(url, strlen(url));
	uut.UrlParser<Uut>::done();
	CHECK(uut.done);
	CHECK(strcmp(uut.path, "/path/filename") == 0);
}

TEST(UrlParser, Badass) {
	const char* url = "scheme://prefix.domain:1234/path/filename?param1=7&param2=seven?.#?junk";

	for(unsigned int i=0; i<strlen(url); i++) {
		uut.reset();
		uut.parseUrl(url, i);
		uut.parseUrl(url + i, strlen(url) - i);
		uut.UrlParser<Uut>::done();

		CHECK(uut.done);
		CHECK(strcmp(uut.path, "/path/filename") == 0);

		CHECK(uut.qdone);
		CHECK(strcmp(uut.query, "param1=7&param2=seven?.") == 0);
	}
}
