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

#include "1test/Test.h"
#include "1test/Mock.h"

#include "HttpLogic.h"
#include "MockedProviders.h"

TEST_GROUP(HttpLogicNormal) {

	MockedHttpLogic uut;

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
		MOCK(ResourceLocator)::EXPECT(reset);
		MOCK(ResourceLocator)::EXPECT(enter).withStringParam("foo");
		MOCK(ContentProvider)::EXPECT(receiveInto).withStringParam("/foo/bar");
		MOCK(ContentProvider)::EXPECT(contentWritten);

		uut.reset();
		uut.parse(testRequest, i);
		uut.parse(testRequest + i, strlen(testRequest) - i);
		uut.done();
		CHECK(uut.getAuthStatus() == MockedHttpLogic::AuthStatus::None);
		CHECK(uut.getStatus() == HTTP_STATUS_OK);
		CHECK(MockedHttpLogic::content == "BodyTest");
	}
}

TEST(HttpLogicNormal, KeepAlive)
{
	static constexpr const char* testRequest =
			"PUT /foo/bar HTTP/1.1\r\n"
			"Connection:keep-alive\r\n"
			"Content-Length:9\r\n\r\n"
			"FirstBody\r\n"
			"PUT /foo/bar HTTP/1.1\r\n"
			"Connection:keep-alive\r\n"
			"Content-Length:10\r\n\r\n"
			"SecondBody\r\n";

	for(unsigned int i=0; i<strlen(testRequest); i++) {
		MOCK(ResourceLocator)::EXPECT(reset);
		MOCK(ResourceLocator)::EXPECT(enter).withStringParam("foo");
		MOCK(ContentProvider)::EXPECT(receiveInto).withStringParam("/foo/bar");
		MOCK(ContentProvider)::EXPECT(contentWritten);

		MOCK(ResourceLocator)::EXPECT(reset);
		MOCK(ResourceLocator)::EXPECT(enter).withStringParam("foo");
		MOCK(ContentProvider)::EXPECT(receiveInto).withStringParam("/foo/bar");
		MOCK(ContentProvider)::EXPECT(contentWritten);

		uut.reset();
		uut.parse(testRequest, i);
		uut.parse(testRequest + i, strlen(testRequest) - i);
		uut.done();
		CHECK(uut.getStatus() == HTTP_STATUS_OK);
		CHECK(MockedHttpLogic::content == "SecondBody");
	}
}

TEST(HttpLogicNormal, Get)
{
	static constexpr const char* testRequest =
			"GET /foo/bar?opt=val#frag HTTP/1.1\r\n\r\n";

	for(unsigned int i=0; i<strlen(testRequest); i++) {
		MOCK(ResourceLocator)::EXPECT(reset);
		MOCK(ResourceLocator)::EXPECT(enter).withStringParam("foo");
		MOCK(ResourceLocator)::EXPECT(enter).withStringParam("bar");
		MOCK(ContentProvider)::EXPECT(sendFrom).withStringParam("/foo/bar");
		MOCK(ContentProvider)::EXPECT(contentRead);

		uut.reset();
		uut.parse(testRequest, i);
		uut.parse(testRequest + i, strlen(testRequest) - i);
		uut.done();

		CHECK(MockedHttpLogic::workerCalled);
		CHECK(uut.getAuthStatus() == MockedHttpLogic::AuthStatus::None);
		CHECK(uut.getStatus() == HTTP_STATUS_OK);
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
		MOCK(ResourceLocator)::EXPECT(reset);
		MOCK(ContentProvider)::EXPECT(receiveInto).withStringParam("/index.html");
		MOCK(ContentProvider)::EXPECT(contentWritten);

		uut.reset();
		uut.parse(testRequest, i);
		uut.parse(testRequest + i, strlen(testRequest) - i - 3);

		CHECK(uut.getAuthStatus() == MockedHttpLogic::AuthStatus::Ok);
		CHECK(uut.getStatus() == HTTP_STATUS_OK);

		uut.parse(testRequest + strlen(testRequest) - 3, 1);
		uut.done();

		CHECK(MockedHttpLogic::workerCalled);
		CHECK(MockedHttpLogic::content == "BodyTest");
	}
}

