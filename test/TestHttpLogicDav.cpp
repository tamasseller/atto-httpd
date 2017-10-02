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
#include "1test/Mock.h"

#include "HttpLogic.h"
#include "MockedProviders.h"

TEST_GROUP(HttpLogicDav) {
	MockedHttpLogic uut;

	TEST_SETUP() {
		MockedHttpLogic::errAt = MockedHttpLogic::ErrAt::None;
		MockedHttpLogic::workerCalled = false;
	}
};

TEST(HttpLogicDav, Copy)
{
	static constexpr const char* testRequest =
			"COPY /foo/bar HTTP/1.1\r\n"
			"Destination: http://127.0.0.1/foo/baz\r\n"
			"Overwrite: T\r\n\r\n";

	MOCK(ResourceLocator)::EXPECT(reset);
	MOCK(ResourceLocator)::EXPECT(enter).withStringParam("foo");
	MOCK(ResourceLocator)::EXPECT(enter).withStringParam("bar");
	MOCK(ResourceLocator)::EXPECT(reset);
	MOCK(ResourceLocator)::EXPECT(enter).withStringParam("foo");
	MOCK(ContentProvider)::EXPECT(copy)
			.withStringParam("/foo/bar")
			.withStringParam("/foo/baz")
			.withParam(true);

	uut.reset();
	uut.parse(testRequest, strlen(testRequest));
	uut.done();
	CHECK(uut.getAuthStatus() == MockedHttpLogic::AuthStatus::None);
	CHECK(uut.getStatus() == HTTP_STATUS_OK);
}

TEST(HttpLogicDav, Move)
{
	static constexpr const char* testRequest =
			"MOVE /foo/bar HTTP/1.1\r\n"
			"Destination: http://127.0.0.1/foo/baz\r\n\r\n";

	MOCK(ResourceLocator)::EXPECT(reset);
	MOCK(ResourceLocator)::EXPECT(enter).withStringParam("foo");
	MOCK(ResourceLocator)::EXPECT(enter).withStringParam("bar");
	MOCK(ResourceLocator)::EXPECT(reset);
	MOCK(ResourceLocator)::EXPECT(enter).withStringParam("foo");
	MOCK(ContentProvider)::EXPECT(move)
			.withStringParam("/foo/bar")
			.withStringParam("/foo/baz")
			.withParam(false);

	uut.reset();
	uut.parse(testRequest, strlen(testRequest));
	uut.done();
	CHECK(uut.getAuthStatus() == MockedHttpLogic::AuthStatus::None);
	CHECK(uut.getStatus() == HTTP_STATUS_OK);
}

TEST(HttpLogicDav, MkCol)
{
	static constexpr const char* testRequest =
			"MKCOL /foo/bar HTTP/1.1\r\n\r\n";

	MOCK(ResourceLocator)::EXPECT(reset);
	MOCK(ResourceLocator)::EXPECT(enter).withStringParam("foo");
	MOCK(ContentProvider)::EXPECT(createDirectory)
			.withStringParam("/foo/bar");

	uut.reset();
	uut.parse(testRequest, strlen(testRequest));
	uut.done();
	CHECK(uut.getAuthStatus() == MockedHttpLogic::AuthStatus::None);
	CHECK(uut.getStatus() == HTTP_STATUS_OK);
}

TEST(HttpLogicDav, Delete)
{
	static constexpr const char* testRequest =
			"DELETE /foo/bar HTTP/1.1\r\n\r\n";

	MOCK(ResourceLocator)::EXPECT(reset);
	MOCK(ResourceLocator)::EXPECT(enter).withStringParam("foo");
	MOCK(ContentProvider)::EXPECT(remove)
			.withStringParam("/foo/bar");

	uut.reset();
	uut.parse(testRequest, strlen(testRequest));
	uut.done();
	CHECK(uut.getAuthStatus() == MockedHttpLogic::AuthStatus::None);
	CHECK(uut.getStatus() == HTTP_STATUS_OK);
}

TEST(HttpLogicDav, PropfindFile)
{
	static constexpr const char* testRequest =
		"PROPFIND / HTTP/1.1\r\n"
		"Depth: 0\r\n"
		"Content-Length: 288\r\n"
		"Content-Type: application/xml\r\n"
		"Authorization: Digest"
		" username=\"foo\",\r\n"
		" realm=\"bar\",\r\n"
		" nonce=\"justonce\",\r\n"
		" uri=\"/\",\r\n"
		" response=\"9edbbf5afdd2e426298f7be121e652ad\",\r\n"
		" algorithm=\"MD5\"\r\n"
		"\r\n"
		"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
		"<propfind xmlns=\"DAV:\"><prop>\n"
		"<getcontentlength xmlns=\"DAV:\"/>\n"
		"<getlastmodified xmlns=\"DAV:\"/>\n"
		"<executable xmlns=\"http://apache.org/dav/props/\"/>\n"
		"<resourcetype xmlns=\"DAV:\"/>\n"
		"<checked-in xmlns=\"DAV:\"/>\n"
		"<checked-out xmlns=\"DAV:\"/>\n"
		"</prop></propfind>\n";

	MOCK(ResourceLocator)::EXPECT(reset);
	MOCK(ContentProvider)::EXPECT(listFile)
			.withStringParam("");
	MOCK(ContentProvider)::EXPECT(listingDone);

	uut.reset();
	uut.parse(testRequest, strlen(testRequest) - 1);
	CHECK(uut.getAuthStatus() == MockedHttpLogic::AuthStatus::Ok);
	CHECK(uut.getStatus() == HTTP_STATUS_OK);
	uut.parse(testRequest + strlen(testRequest) - 1, 1);
	uut.done();
	CHECK(MockedHttpLogic::workerCalled);

}

IGNORE_TEST(HttpLogicDav, PropfindDir)
{
	static constexpr const char* testRequest =
		"PROPFIND / HTTP/1.1\r\n"
		"Depth: 1\r\n\r\n";

	MOCK(ResourceLocator)::EXPECT(reset);
	MOCK(ContentProvider)::EXPECT(listDirectory)
			.withStringParam("");
	MOCK(ContentProvider)::EXPECT(listingDone);

	uut.reset();
	uut.parse(testRequest, strlen(testRequest));
	uut.done();

	CHECK(MockedHttpLogic::workerCalled);
	CHECK(uut.getAuthStatus() == MockedHttpLogic::AuthStatus::None);
	CHECK(uut.getStatus() == HTTP_STATUS_OK);
}
