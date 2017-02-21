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

#include "HttpLogic.h"
#include "MockedProviders.h"

TEST_GROUP(HttpLogicDav) {
	MockedHttpLogic uut;

	TEST_TEARDOWN() {
		mock().checkExpectations();
		mock().clear();
	}

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

	mock("ResourceLocator").expectOneCall("reset");
	mock("ResourceLocator").expectOneCall("enter").withStringParameter("str", "foo");
	mock("ResourceLocator").expectOneCall("enter").withStringParameter("str", "bar");
	mock("ResourceLocator").expectOneCall("reset");
	mock("ResourceLocator").expectOneCall("enter").withStringParameter("str", "foo");
	mock("ContentProvider").expectOneCall("copy")
			.withStringParameter("src", "/foo/bar")
			.withStringParameter("dst", "/foo/baz")
			.withBoolParameter("overwrite", true);

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

	mock("ResourceLocator").expectOneCall("reset");
	mock("ResourceLocator").expectOneCall("enter").withStringParameter("str", "foo");
	mock("ResourceLocator").expectOneCall("enter").withStringParameter("str", "bar");
	mock("ResourceLocator").expectOneCall("reset");
	mock("ResourceLocator").expectOneCall("enter").withStringParameter("str", "foo");
	mock("ContentProvider").expectOneCall("move")
			.withStringParameter("src", "/foo/bar")
			.withStringParameter("dst", "/foo/baz")
			.withBoolParameter("overwrite", false);

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

	mock("ResourceLocator").expectOneCall("reset");
	mock("ResourceLocator").expectOneCall("enter").withStringParameter("str", "foo");
	mock("ContentProvider").expectOneCall("createDirectory")
			.withStringParameter("path", "/foo/bar");

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

	mock("ResourceLocator").expectOneCall("reset");
	mock("ResourceLocator").expectOneCall("enter").withStringParameter("str", "foo");
	mock("ContentProvider").expectOneCall("remove")
			.withStringParameter("path", "/foo/bar");

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

	mock("ResourceLocator").expectOneCall("reset");
	mock("ContentProvider").expectOneCall("listFile")
			.withStringParameter("path", "");
	mock("ContentProvider").expectOneCall("listingDone");

	uut.reset();
	uut.parse(testRequest, strlen(testRequest) - 1);
	CHECK(uut.getAuthStatus() == MockedHttpLogic::AuthStatus::Ok);
	CHECK(uut.getStatus() == HTTP_STATUS_OK);
	uut.parse(testRequest + strlen(testRequest) - 1, 1);
	uut.done();
	CHECK(MockedHttpLogic::workerCalled);

}

TEST(HttpLogicDav, PropfindDir)
{
	static constexpr const char* testRequest =
		"PROPFIND / HTTP/1.1\r\n"
		"Depth: 1\r\n\r\n";

	mock("ResourceLocator").expectOneCall("reset");
	mock("ContentProvider").expectOneCall("listDirectory")
			.withStringParameter("path", "");
	mock("ContentProvider").expectOneCall("listingDone");

	uut.reset();
	uut.parse(testRequest, strlen(testRequest));
	uut.done();

	CHECK(MockedHttpLogic::workerCalled);
	CHECK(uut.getAuthStatus() == MockedHttpLogic::AuthStatus::None);
	CHECK(uut.getStatus() == HTTP_STATUS_OK);
}
