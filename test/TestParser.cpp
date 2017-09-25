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

#include "HttpRequestParser.h"

#include <string>
#include <cstring>

class MockedParser: public HttpRequestParser<MockedParser> {
	friend HttpRequestParser<MockedParser>;

	inline void beforeRequest() {
		MOCK(MockedParser)::CALL(beforeRequest);
	}

	inline void beforeUrl() {
		MOCK(MockedParser)::CALL(beforeUrl);
	}

	inline int onUrl(const char *at, size_t length) {
		MOCK(MockedParser)::CALL(onUrl)
				.withStringParam(std::string(at, length).c_str());

		return 0;
	}

	inline void afterUrl() {
		MOCK(MockedParser)::CALL(afterUrl);
	}

	inline void beforeHeaderName() {
		MOCK(MockedParser)::CALL(beforeHeaderName);
	}

	inline int onHeaderName(const char *at, size_t length) {
		MOCK(MockedParser)::CALL(onHeaderName)
				.withStringParam(std::string(at, length).c_str());

		return 0;
	}

	inline void afterHeaderName() {
		MOCK(MockedParser)::CALL(afterHeaderName);
	}

	inline void beforeHeaderValue() {
		MOCK(MockedParser)::CALL(beforeHeaderValue);
	}

	inline int onHeaderValue(const char *at, size_t length) {
		MOCK(MockedParser)::CALL(onHeaderValue)
				.withStringParam(std::string(at, length).c_str());

		return 0;
	}

	inline void afterHeaderValue() {
		MOCK(MockedParser)::CALL(afterHeaderValue);
	}

	inline void beforeBody() {
		MOCK(MockedParser)::CALL(beforeBody);
	}

	inline int onBody(const char *at, size_t length) {
		MOCK(MockedParser)::CALL(onBody)
				.withStringParam(std::string(at, length).c_str());

		return 0;
	}

	inline void afterBody() {
		MOCK(MockedParser)::CALL(afterBody);
	}

	inline void afterRequest() {
		MOCK(MockedParser)::CALL(afterRequest);
	}
};

TEST_GROUP(HttpRequestParser) {
	MockedParser parser;

	TEST_SETUP() {
		parser.reset();
	}
};

TEST(HttpRequestParser, Simple) {
	static constexpr const char* request = "GET / HTTP/1.1\r\n\r\n";

	MOCK(MockedParser)::EXPECT(beforeRequest);
	MOCK(MockedParser)::EXPECT(beforeUrl);
	MOCK(MockedParser)::EXPECT(onUrl).withStringParam("/");
	MOCK(MockedParser)::EXPECT(afterUrl);
	MOCK(MockedParser)::EXPECT(afterRequest);

	CHECK(parser.parse(request, std::strlen(request)) == std::strlen(request));
	parser.done();
}

TEST(HttpRequestParser, WithUrl) {
	static constexpr const char* request = "GET /u/r/l HTTP/1.1\r\n\r\n";

	MOCK(MockedParser)::EXPECT(beforeRequest);
	MOCK(MockedParser)::EXPECT(beforeUrl);
	MOCK(MockedParser)::EXPECT(onUrl).withStringParam("/u/r/l");
	MOCK(MockedParser)::EXPECT(afterUrl);
	MOCK(MockedParser)::EXPECT(afterRequest);

	CHECK(parser.parse(request, std::strlen(request)) == std::strlen(request));
	parser.done();
}

TEST(HttpRequestParser, OneHeader) {
	static constexpr const char* request =
			"GET / HTTP/1.1\r\n"
			"Host: asd.qwe\r\n\r\n";

	MOCK(MockedParser)::EXPECT(beforeRequest);

	MOCK(MockedParser)::EXPECT(beforeUrl);
	MOCK(MockedParser)::EXPECT(onUrl).withStringParam("/");
	MOCK(MockedParser)::EXPECT(afterUrl);

	MOCK(MockedParser)::EXPECT(beforeHeaderName);
	MOCK(MockedParser)::EXPECT(onHeaderName).withStringParam("Host");
	MOCK(MockedParser)::EXPECT(afterHeaderName);
	MOCK(MockedParser)::EXPECT(beforeHeaderValue);
	MOCK(MockedParser)::EXPECT(onHeaderValue).withStringParam("asd.qwe");
	MOCK(MockedParser)::EXPECT(afterHeaderValue);

	MOCK(MockedParser)::EXPECT(afterRequest);

	CHECK(parser.parse(request, std::strlen(request)) == std::strlen(request));
	parser.done();
}

TEST(HttpRequestParser, TwoHeaders) {
	static constexpr const char* request =
			"GET / HTTP/1.1\r\n"
			"Host: asd.qwe\r\n"
			"X-My:ass\r\n\r\n";

	MOCK(MockedParser)::EXPECT(beforeRequest);

	MOCK(MockedParser)::EXPECT(beforeUrl);
	MOCK(MockedParser)::EXPECT(onUrl).withStringParam("/");
	MOCK(MockedParser)::EXPECT(afterUrl);

	MOCK(MockedParser)::EXPECT(beforeHeaderName);
	MOCK(MockedParser)::EXPECT(onHeaderName).withStringParam("Host");
	MOCK(MockedParser)::EXPECT(afterHeaderName);
	MOCK(MockedParser)::EXPECT(beforeHeaderValue);
	MOCK(MockedParser)::EXPECT(onHeaderValue).withStringParam("asd.qwe");
	MOCK(MockedParser)::EXPECT(afterHeaderValue);

	MOCK(MockedParser)::EXPECT(beforeHeaderName);
	MOCK(MockedParser)::EXPECT(onHeaderName).withStringParam("X-My");
	MOCK(MockedParser)::EXPECT(afterHeaderName);
	MOCK(MockedParser)::EXPECT(beforeHeaderValue);
	MOCK(MockedParser)::EXPECT(onHeaderValue).withStringParam("ass");
	MOCK(MockedParser)::EXPECT(afterHeaderValue);

	MOCK(MockedParser)::EXPECT(afterRequest);

	CHECK(parser.parse(request, std::strlen(request)) == std::strlen(request));
	parser.done();
}

TEST(HttpRequestParser, TwoHeaderWithBody) {
	static constexpr const char* request =
			"GET / HTTP/1.1\r\n"
			"Host: asd.qwe\r\n"
			"Content-Length:8\r\n\r\n"
			"BodyTest\r\n";

	MOCK(MockedParser)::EXPECT(beforeRequest);

	MOCK(MockedParser)::EXPECT(beforeUrl);
	MOCK(MockedParser)::EXPECT(onUrl).withStringParam("/");
	MOCK(MockedParser)::EXPECT(afterUrl);

	MOCK(MockedParser)::EXPECT(beforeHeaderName);
	MOCK(MockedParser)::EXPECT(onHeaderName).withStringParam("Host");
	MOCK(MockedParser)::EXPECT(afterHeaderName);
	MOCK(MockedParser)::EXPECT(beforeHeaderValue);
	MOCK(MockedParser)::EXPECT(onHeaderValue).withStringParam("asd.qwe");
	MOCK(MockedParser)::EXPECT(afterHeaderValue);

	MOCK(MockedParser)::EXPECT(beforeHeaderName);
	MOCK(MockedParser)::EXPECT(onHeaderName).withStringParam("Content-Length");
	MOCK(MockedParser)::EXPECT(afterHeaderName);
	MOCK(MockedParser)::EXPECT(beforeHeaderValue);
	MOCK(MockedParser)::EXPECT(onHeaderValue).withStringParam("8");
	MOCK(MockedParser)::EXPECT(afterHeaderValue);

	MOCK(MockedParser)::EXPECT(beforeBody);
	MOCK(MockedParser)::EXPECT(onBody).withStringParam("BodyTest");
	MOCK(MockedParser)::EXPECT(afterBody);

	MOCK(MockedParser)::EXPECT(afterRequest);

	CHECK(parser.parse(request, std::strlen(request)) == std::strlen(request));
	parser.done();
}

class MockedParserPartial: public HttpRequestParser<MockedParserPartial> {
	friend HttpRequestParser<MockedParserPartial>;

	inline int onUrl(const char *at, size_t length) {
		MOCK(MockedParser)::CALL(onUrl)
				.withStringParam(std::string(at, length).c_str());

		return 0;
	}

	inline int onHeaderName(const char *at, size_t length) {
		MOCK(MockedParser)::CALL(onHeaderName)
				.withStringParam(std::string(at, length).c_str());

		return 0;
	}

	inline int onBody(const char *at, size_t length) {
		MOCK(MockedParser)::CALL(onBody)
				.withStringParam(std::string(at, length).c_str());

		return 0;
	}

};

TEST_GROUP(HttpRequestParserPartial) {
	MockedParserPartial parser;

	TEST_SETUP() {
		parser.reset();
	}
};

TEST(HttpRequestParserPartial, TwoHeaderWithBody) {
	static constexpr const char* request =
			"GET / HTTP/1.1\r\n"
			"Host: asd.qwe\r\n"
			"Content-Length:8\r\n\r\n"
			"BodyTest\r\n";

	MOCK(MockedParser)::EXPECT(onUrl).withStringParam("/");
	MOCK(MockedParser)::EXPECT(onHeaderName).withStringParam("Host");
	MOCK(MockedParser)::EXPECT(onHeaderName).withStringParam("Content-Length");
	MOCK(MockedParser)::EXPECT(onBody).withStringParam("BodyTest");

	CHECK(parser.parse(request, std::strlen(request)) == std::strlen(request));
	parser.done();
}

TEST(HttpRequestParserPartial, Bullshit) {
	static constexpr const char* request =
		"\x36\xd6\xb4\x27\x0d\xe9\x37\x23\x2f\xf4\x27\x39\x8f\xb4\x50"
		"\xc6\x2e\x68\x42\xd1\xed\x08\xf0\x68\xe1\xb7\x96\xa7\x38\x45"
		"\xf0\x86\xf2\xb8\x58\x96\x35\x5c\xe1\xa5\x8a\x9c\x67\xf6\xef"
		"\x80\x90\xb0\x02\xac\xe0\x6a\x0d\xb8\xc6\xc1\x5c\x83\xc9\x3b";

	parser.parse(request, std::strlen(request));
	parser.done();
}

TEST(HttpRequestParserPartial, BadRequest) {
	static constexpr const char* request =
			"HACK / HTTP/1.1\r\n"
			"Content-Length:8\r\n\r\n"
			"BodyTest\r\n";

	parser.parse(request, std::strlen(request));
	parser.done();
}

TEST(HttpRequestParserPartial, BadUri) {
	static constexpr const char* request =
			"GET /invalid uri HTTP/1.1\r\n"
			"BodyTest\r\n";

	MOCK(MockedParser)::EXPECT(onUrl).withStringParam("/invalid");

	parser.parse(request, std::strlen(request));
	parser.done();
}

struct SavingParser: public HttpRequestParser<SavingParser> {
	char url[256], name[256], value[256], body[256];
	int beforeRequestCalled;
	int beforeUrlCalled;
	int afterUrlCalled;
	int beforeHeaderNameCalled;
	int afterHeaderNameCalled;
	int beforeHeaderValueCalled;
	int afterHeaderValueCalled;
	int beforeBodyCalled;
	int afterBodyCalled;
	int afterRequestCalled;

	void reset() {
		HttpRequestParser<SavingParser>::reset();
		bzero(url, sizeof(url));
		bzero(name, sizeof(name));
		bzero(value, sizeof(value));
		bzero(body, sizeof(body));
		beforeRequestCalled = 0;
		beforeUrlCalled = 0;
		afterUrlCalled = 0;
		beforeHeaderNameCalled = 0;
		afterHeaderNameCalled = 0;
		beforeHeaderValueCalled = 0;
		afterHeaderValueCalled = 0;
		beforeBodyCalled = 0;
		afterBodyCalled = 0;
		afterRequestCalled = 0;
	}

	inline void beforeRequest() {beforeRequestCalled++;}
	inline void beforeUrl() {beforeUrlCalled++;}
	inline void afterUrl() {afterUrlCalled++;}
	inline void beforeHeaderName() {beforeHeaderNameCalled++;}
	inline void afterHeaderName() {afterHeaderNameCalled++;}
	inline void beforeHeaderValue() {beforeHeaderValueCalled++;}
	inline void afterHeaderValue() {afterHeaderValueCalled++;}
	inline void beforeBody() {beforeBodyCalled++;}
	inline void afterBody() {afterBodyCalled++;}
	inline void afterRequest() {afterRequestCalled++;}

	inline int onUrl(const char *at, size_t length) {
		strncat(url, at, length);
		return 0;
	}

	inline int onHeaderName(const char *at, size_t length) {
		strncat(name, at, length);
		return 0;
	}

	inline int onHeaderValue(const char *at, size_t length) {
		strncat(value, at, length);
		return 0;
	}

	inline int onBody(const char *at, size_t length) {
		strncat(body, at, length);
		return 0;
	}
};


TEST_GROUP(HttpRequestParserSegmented) {
	SavingParser parser;
};

TEST(HttpRequestParserSegmented, Segmented) {
	static constexpr const char* request =
			"GET /segmented HTTP/1.1\r\n"
			"Host: asd.qwe\r\n"
			"Content-Length:8\r\n\r\n"
			"BodyTest\r\n";

	for(unsigned int i=0; i<strlen(request); i++) {
		parser.reset();
		CHECK(parser.parse(request, i) == (int)i);
		CHECK(parser.parse(request + i, std::strlen(request) - i) == (int)(std::strlen(request) - i));
		parser.done();

		CHECK(parser.beforeRequestCalled == 1);
		CHECK(parser.beforeUrlCalled == 1);
		CHECK(parser.afterUrlCalled == 1);
		CHECK(parser.beforeHeaderNameCalled == 2);
		CHECK(parser.afterHeaderNameCalled == 2);
		CHECK(parser.beforeHeaderValueCalled == 2);
		CHECK(parser.afterHeaderValueCalled == 2);
		CHECK(parser.beforeBodyCalled == 1);
		CHECK(parser.afterBodyCalled == 1);
		CHECK(parser.afterRequestCalled == 1);
		CHECK(strcmp(parser.url, "/segmented") == 0);
		CHECK(strcmp(parser.name, "HostContent-Length") == 0);
		CHECK(strcmp(parser.value, "asd.qwe8") == 0);
		CHECK(strcmp(parser.body, "BodyTest") == 0);
	}
}

TEST(HttpRequestParser, ForCoverage) {
	CHECK(!MockedParser::getMethodText((MockedParser::Method)666));
}
