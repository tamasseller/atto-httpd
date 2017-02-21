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
 *******************************************************************************/

#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "HttpLogic.h"
#include "MockedProviders.h"

TEST_GROUP(HttpLogicNormal) {

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

TEST(HttpLogicNormal, Put)
{
	static constexpr const char* testRequest =
			"PUT /foo/bar HTTP/1.1\r\n"
			"Content-Length:8\r\n\r\n"
			"BodyTest\r\n";

	for(unsigned int i=0; i<strlen(testRequest); i++) {
		mock().clear();
		mock("ResourceLocator").expectOneCall("reset");
		mock("ResourceLocator").expectOneCall("enter").withStringParameter("str", "foo");
		mock("ContentProvider").expectOneCall("receiveInto").withStringParameter("path", "/foo/bar");
		mock("ContentProvider").expectOneCall("contentWritten");

		uut.reset();
		uut.parse(testRequest, i);
		uut.parse(testRequest + i, strlen(testRequest) - i);
		uut.done();
		CHECK(uut.getAuthStatus() == MockedHttpLogic::AuthStatus::None);
		CHECK(uut.getStatus() == HTTP_STATUS_OK);
		CHECK(MockedHttpLogic::content == "BodyTest");
		mock().checkExpectations();
	}
}

TEST(HttpLogicNormal, Get)
{
	static constexpr const char* testRequest =
			"GET /foo/bar?opt=val#frag HTTP/1.1\r\n\r\n";

	for(unsigned int i=0; i<strlen(testRequest); i++) {
		mock().clear();

		mock("ResourceLocator").expectOneCall("reset");
		mock("ResourceLocator").expectOneCall("enter").withStringParameter("str", "foo");
		mock("ResourceLocator").expectOneCall("enter").withStringParameter("str", "bar");
		mock("ContentProvider").expectOneCall("sendFrom").withStringParameter("path", "/foo/bar");
		mock("ContentProvider").expectOneCall("contentRead");

		uut.reset();
		uut.parse(testRequest, i);
		uut.parse(testRequest + i, strlen(testRequest) - i);
		uut.done();

		CHECK(MockedHttpLogic::workerCalled);
		CHECK(uut.getAuthStatus() == MockedHttpLogic::AuthStatus::None);
		CHECK(uut.getStatus() == HTTP_STATUS_OK);
		mock().checkExpectations();
	}
}

TEST(HttpLogicNormal, PutAuth)
{
	static constexpr const char* testRequest =
			"PUT /index.html HTTP/1.1\r\n"
			"Authorization: Digest\r\n"
			" username=\"foo\",\r\n"
			" realm=\"bar\",\r\n"
			" nonce=\"justonce\",\r\n"
			" uri=\"/index.html\",\r\n"
			" response=\"3f698a1237eeb963bb25fed3febf42de\",\r\n"
			" algorithm=\"MD5\"\r\n"
			"Content-Length:8\r\n\r\n"
			"BodyTest\r\n";

	for(unsigned int i=0; i<strlen(testRequest) - 3; i++) {
		mock().clear();

		mock("ResourceLocator").expectOneCall("reset");
		mock("ContentProvider").expectOneCall("receiveInto").withStringParameter("path", "/index.html");
		mock("ContentProvider").expectOneCall("contentWritten");

		uut.reset();
		uut.parse(testRequest, i);
		uut.parse(testRequest + i, strlen(testRequest) - i - 3);

		CHECK(uut.getAuthStatus() == MockedHttpLogic::AuthStatus::Ok);
		CHECK(uut.getStatus() == HTTP_STATUS_OK);

		uut.parse(testRequest + strlen(testRequest) - 3, 1);
		uut.done();

		CHECK(MockedHttpLogic::workerCalled);
		CHECK(MockedHttpLogic::content == "BodyTest");
		mock().checkExpectations();
	}
}

