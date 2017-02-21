/*
 * TestHttpLogic.cpp
 *
 *  Created on: 2017.02.09.
 *      Author: tooma
 */

#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "HttpLogic.h"
#include "MockedProviders.h"

TEST_GROUP(HttpLogicErrors) {

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
		mock().clear();

		mock("ResourceLocator").expectOneCall("reset");

		uut.reset();
		uut.parse(testRequest, i);
		uut.parse(testRequest + i, strlen(testRequest) - i - 1);

		CHECK(uut.getAuthStatus() == MockedHttpLogic::AuthStatus::Failed);
		CHECK(uut.getStatus() == HTTP_STATUS_UNAUTHORIZED);
		uut.parse(testRequest + strlen(testRequest) - 1, 1);

		uut.done();
		CHECK(MockedHttpLogic::content == "");
		mock().checkExpectations();
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
		mock().clear();

		mock("ResourceLocator").expectOneCall("reset");
		mock("ContentProvider").expectOneCall("receiveInto").withStringParameter("path", "/index.html");

		uut.reset();
		uut.parse(testRequest, i);
		uut.parse(testRequest + i, strlen(testRequest) - i);
		uut.done();
		CHECK(uut.getAuthStatus() == MockedHttpLogic::AuthStatus::Ok);
		CHECK(uut.getStatus() == HTTP_STATUS_BAD_REQUEST);
		mock().checkExpectations();
	}
}

TEST(HttpLogicErrors, PropfindInvalid)
{
	static constexpr const char* testRequest =
		"PROPFIND / HTTP/1.1\r\n"
		"Depth: Invalid\r\n\r\n"
		"garbage";

	mock("ResourceLocator").expectOneCall("reset");

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

	mock("ResourceLocator").disable();

	MockedHttpLogic::errAt = MockedHttpLogic::ErrAt::CloseForWriting;

	mock("ContentProvider").expectOneCall("receiveInto").withStringParameter("path", "/index.html");
	mock("ContentProvider").expectOneCall("contentWritten");

	uut.reset();
	uut.parse(testRequest, strlen(testRequest));
	uut.done();

	mock().checkExpectations();

	//////////

	MockedHttpLogic::errAt = MockedHttpLogic::ErrAt::Writing;

	mock("ContentProvider").expectOneCall("receiveInto").withStringParameter("path", "/index.html");

	uut.reset();
	uut.parse(testRequest, strlen(testRequest) - 3);
	CHECK(uut.getStatus() == HTTP_STATUS_FORBIDDEN);
	uut.parse(testRequest + strlen(testRequest) - 3, 3);
	uut.done();

	mock().checkExpectations();

	//////////

	MockedHttpLogic::errAt = MockedHttpLogic::ErrAt::OpenForWriting;

	mock("ContentProvider").expectOneCall("receiveInto").withStringParameter("path", "/index.html");

	uut.reset();
	uut.parse(testRequest, strlen(testRequest) - 3);
	CHECK(uut.getStatus() == HTTP_STATUS_FORBIDDEN);
	uut.parse(testRequest + strlen(testRequest) - 3, 3);
	uut.done();

	mock().checkExpectations();
	mock().clear();

	mock("ResourceLocator").enable();
}

TEST(HttpLogicErrors, GetWithErrors)
{
	static constexpr const char* testRequest = "GET /foo/bar HTTP/1.1\r\n\r\ngarbage";
	mock("ResourceLocator").disable();

	MockedHttpLogic::errAt = MockedHttpLogic::ErrAt::CloseForReading;

	mock("ContentProvider").expectOneCall("sendFrom").withStringParameter("path", "/foo/bar");
	mock("ContentProvider").expectOneCall("contentRead");

	uut.reset();
	uut.parse(testRequest, strlen(testRequest));
	uut.done();

	mock().checkExpectations();

	//////////

	MockedHttpLogic::errAt = MockedHttpLogic::ErrAt::Reading;

	mock("ContentProvider").expectOneCall("sendFrom").withStringParameter("path", "/foo/bar");

	uut.reset();
	uut.parse(testRequest, strlen(testRequest));
	uut.done();

	mock().checkExpectations();

	//////////

	MockedHttpLogic::errAt = MockedHttpLogic::ErrAt::OpenForReading;

	mock("ContentProvider").expectOneCall("sendFrom").withStringParameter("path", "/foo/bar");

	uut.reset();
	uut.parse(testRequest, strlen(testRequest) - 2);
	uut.parse(testRequest + strlen(testRequest) - 2, 2);
	uut.done();

	mock().checkExpectations();
	mock().clear();

	mock("ResourceLocator").enable();
}


TEST(HttpLogicErrors, LsWithErrors)
{
	static constexpr const char* testRequest =
		"PROPFIND / HTTP/1.1\r\n"
		"Depth: 1\r\n\r\n";

	mock("ResourceLocator").disable();

	MockedHttpLogic::errAt = MockedHttpLogic::ErrAt::CloseForListing;

	mock("ContentProvider").expectOneCall("listDirectory").withStringParameter("path", "");
	mock("ContentProvider").expectOneCall("listingDone");

	uut.reset();
	uut.parse(testRequest, strlen(testRequest));
	uut.done();

	mock().checkExpectations();

	//////////

	MockedHttpLogic::errAt = MockedHttpLogic::ErrAt::Listing;

	mock("ContentProvider").expectOneCall("listDirectory").withStringParameter("path", "");

	uut.reset();
	uut.parse(testRequest, strlen(testRequest));
	uut.done();

	mock().checkExpectations();

	//////////

	MockedHttpLogic::errAt = MockedHttpLogic::ErrAt::OpenForListing;

	mock("ContentProvider").expectOneCall("listDirectory").withStringParameter("path", "");

	uut.reset();
	uut.parse(testRequest, strlen(testRequest));
	uut.done();

	mock().checkExpectations();
	mock().clear();

	mock("ResourceLocator").enable();
}
