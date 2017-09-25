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

TEST_GROUP(HttpLogicErrors) {

	MockedHttpLogic uut;

	TEST_SETUP() {
		MockedHttpLogic::errAt = MockedHttpLogic::ErrAt::None;
		MockedHttpLogic::workerCalled = false;
	}
};

TEST(HttpLogicErrors, PutWrongAuth)
{
	static constexpr const char* testRequest =
			"PUT /index.html HTTP/1.1\r\n"
			"Authorization: Digest\r\n"
			" username=\"BADGUY\",\r\n"
			" realm=\"bar\",\r\n"
			" nonce=\"justonce\",\r\n"
			" uri=\"/index.html\",\r\n"
			" response=\"3f698a1237eeb963bb25fed3febf42de\",\r\n"
			" algorithm=\"MD5\"\r\n"
			"Content-Length:8\r\n\r\n"
			"BodyTest";

	for(unsigned int i=0; i<strlen(testRequest) - 1; i++) {
		MOCK(ResourceLocator)::EXPECT(reset);

		uut.reset();
		uut.parse(testRequest, i);
		uut.parse(testRequest + i, strlen(testRequest) - i - 1);

		CHECK(uut.getAuthStatus() == MockedHttpLogic::AuthStatus::Failed);
		CHECK(uut.getStatus() == HTTP_STATUS_UNAUTHORIZED);
		uut.parse(testRequest + strlen(testRequest) - 1, 1);

		uut.done();
		CHECK(MockedHttpLogic::content == "");
	}
}

TEST(HttpLogicErrors, PutBadReq)
{
	static constexpr const char* testRequest =
			"HACK /index.html HTTP/1.1\r\n"
			"Authorization: Digest\r\n"
			" username=\"foo\",\r\n"
			" realm=\"bar\",\r\n"
			" nonce=\"justonce\",\r\n"
			" uri=\"/index.html\",\r\n"
			" response=\"3f698a1237eeb963bb25fed3febf42de\",\r\n"
			"Content-Length:8\r\n\r\n"
			"BodyTest";

	for(unsigned int i=0; i<strlen(testRequest) - 1; i++) {
		uut.reset();
		uut.parse(testRequest, i);
		uut.parse(testRequest + i, strlen(testRequest) - i - 1);

		CHECK(uut.getAuthStatus() == MockedHttpLogic::AuthStatus::None);
		CHECK(uut.getStatus() == HTTP_STATUS_BAD_REQUEST);

		uut.parse(testRequest + strlen(testRequest) - 1, 1);
		uut.done();
	}
}

TEST(HttpLogicErrors, PutTrunactedReq)
{
	static constexpr const char* testRequest =
			"PUT /index.html HTTP/1.1\r\n"
			"Authorization: Digest\r\n"
			" username=\"foo\",\r\n"
			" realm=\"bar\",\r\n"
			" nonce=\"justonce\",\r\n"
			" uri=\"/index.html\",\r\n"
			" response=\"3f698a1237eeb963bb25fed3febf42de\",\r\n"
			"Content-Length:8\r\n\r\n"
			"Body";

	for(unsigned int i=0; i<strlen(testRequest); i++) {
		MOCK(ResourceLocator)::EXPECT(reset);
		MOCK(ContentProvider)::EXPECT(receiveInto).withStringParam("/index.html");

		uut.reset();
		uut.parse(testRequest, i);
		uut.parse(testRequest + i, strlen(testRequest) - i);
		uut.done();
		CHECK(uut.getAuthStatus() == MockedHttpLogic::AuthStatus::Ok);
		CHECK(uut.getStatus() == HTTP_STATUS_BAD_REQUEST);
	}
}

TEST(HttpLogicErrors, PropfindInvalid)
{
	static constexpr const char* testRequest =
		"PROPFIND / HTTP/1.1\r\n"
		"Depth: Invalid\r\n\r\n"
		"garbage";

	MOCK(ResourceLocator)::EXPECT(reset);

	uut.reset();
	uut.parse(testRequest, strlen(testRequest) - 3);

	CHECK(uut.getAuthStatus() == MockedHttpLogic::AuthStatus::None);
	CHECK(uut.getStatus() == HTTP_STATUS_BAD_REQUEST);

	uut.parse(testRequest + strlen(testRequest) - 3, 3);
	uut.done();

}

TEST(HttpLogicErrors, PutWithErrors)
{
	static constexpr const char* testRequest =
			"PUT /index.html HTTP/1.1\r\n"
			"Content-Length:8\r\n\r\n"
			"ConTent\n";

	MOCK(ResourceLocator)::disable();

	MockedHttpLogic::errAt = MockedHttpLogic::ErrAt::CloseForWriting;

	MOCK(ContentProvider)::EXPECT(receiveInto).withStringParam("/index.html");
	MOCK(ContentProvider)::EXPECT(contentWritten);

	uut.reset();
	uut.parse(testRequest, strlen(testRequest));
	uut.done();

	//////////

	MockedHttpLogic::errAt = MockedHttpLogic::ErrAt::Writing;

	MOCK(ContentProvider)::EXPECT(receiveInto).withStringParam("/index.html");

	uut.reset();
	uut.parse(testRequest, strlen(testRequest) - 3);
	CHECK(uut.getStatus() == HTTP_STATUS_FORBIDDEN);
	uut.parse(testRequest + strlen(testRequest) - 3, 3);
	uut.done();

	//////////

	MockedHttpLogic::errAt = MockedHttpLogic::ErrAt::OpenForWriting;

	MOCK(ContentProvider)::EXPECT(receiveInto).withStringParam("/index.html");

	uut.reset();
	uut.parse(testRequest, strlen(testRequest) - 3);
	CHECK(uut.getStatus() == HTTP_STATUS_FORBIDDEN);
	uut.parse(testRequest + strlen(testRequest) - 3, 3);
	uut.done();

	MOCK(ResourceLocator)::enable();
}

TEST(HttpLogicErrors, GetWithErrors)
{
	static constexpr const char* testRequest = "GET /foo/bar HTTP/1.1\r\n\r\ngarbage";
	MOCK(ResourceLocator)::disable();

	MockedHttpLogic::errAt = MockedHttpLogic::ErrAt::CloseForReading;

	MOCK(ContentProvider)::EXPECT(sendFrom).withStringParam("/foo/bar");
	MOCK(ContentProvider)::EXPECT(contentRead);

	uut.reset();
	uut.parse(testRequest, strlen(testRequest));
	uut.done();

	//////////

	MockedHttpLogic::errAt = MockedHttpLogic::ErrAt::Reading;

	MOCK(ContentProvider)::EXPECT(sendFrom).withStringParam("/foo/bar");

	uut.reset();
	uut.parse(testRequest, strlen(testRequest));
	uut.done();

	//////////

	MockedHttpLogic::errAt = MockedHttpLogic::ErrAt::OpenForReading;

	MOCK(ContentProvider)::EXPECT(sendFrom).withStringParam("/foo/bar");

	uut.reset();
	uut.parse(testRequest, strlen(testRequest) - 2);
	uut.parse(testRequest + strlen(testRequest) - 2, 2);
	uut.done();

	MOCK(ResourceLocator)::enable();
}


TEST(HttpLogicErrors, LsWithErrors)
{
	static constexpr const char* testRequest =
		"PROPFIND / HTTP/1.1\r\n"
		"Depth: 1\r\n\r\n";

	MOCK(ResourceLocator)::disable();

	MockedHttpLogic::errAt = MockedHttpLogic::ErrAt::CloseForListing;

	MOCK(ContentProvider)::EXPECT(listDirectory).withStringParam("");
	MOCK(ContentProvider)::EXPECT(listingDone);

	uut.reset();
	uut.parse(testRequest, strlen(testRequest));
	uut.done();

	//////////

	MockedHttpLogic::errAt = MockedHttpLogic::ErrAt::Listing;

	MOCK(ContentProvider)::EXPECT(listDirectory).withStringParam("");

	uut.reset();
	uut.parse(testRequest, strlen(testRequest));
	uut.done();

	//////////

	MockedHttpLogic::errAt = MockedHttpLogic::ErrAt::OpenForListing;

	MOCK(ContentProvider)::EXPECT(listDirectory).withStringParam("");

	uut.reset();
	uut.parse(testRequest, strlen(testRequest));
	uut.done();

	MOCK(ResourceLocator)::enable();
}
