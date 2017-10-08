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

#include "HttpLogic.h"

#include <string>

namespace {
	struct Types {
		static constexpr const uint32_t davStackSize = 192;
		static constexpr const char* username = "foo";
		static constexpr const char* realm = "bar";
		static constexpr const char* RFC2069_A1 = "d65f52b42a2605dd84ef29a88bd75e1d";

		static constexpr const DavProperty davProperties[2] = {
			DavProperty("DAV:", "getcontentlength"),
			DavProperty("foo://bar", "otherprop")
		};

	};

	constexpr const DavProperty Types::davProperties[2];
}

TEST_GROUP(HttpLogicOutput) {

	struct Uut: public HttpLogic<Uut, Types> {
		uint32_t n;

		std::string response;

		void send(const char* str, unsigned int length) {
			response += std::string(str, length);
		}

		void flush() {}

		DavAccess sourceAccessible(bool authenticated) { return DavAccess::Dav; }

		void resetSourceLocator() {}
		void resetDestinationLocator() {}
		HttpStatus enterSource(const char* str, unsigned int length) {return HTTP_STATUS_OK;}
		HttpStatus enterDestination(const char* str, unsigned int length) {return HTTP_STATUS_OK;}

		HttpStatus createDirectory(const char* dstName, uint32_t length) {
			return HTTP_STATUS_CREATED;
		}

		HttpStatus remove(const char* dstName, uint32_t length) {
			return HTTP_STATUS_NO_CONTENT;
		}

		HttpStatus copy(const char* dstName, uint32_t length, bool overwrite) {
			return HTTP_STATUS_CREATED;
		}

		HttpStatus move(const char* dstName, uint32_t length, bool overwrite) {
			return HTTP_STATUS_CREATED;
		}

		// Upload

		HttpStatus arrangeReceiveInto(const char* dstName, uint32_t length) {
			return HTTP_STATUS_OK;
		}

		HttpStatus writeContent(const char* buff, uint32_t length) {
			return HTTP_STATUS_OK;
		}

		HttpStatus contentWritten() {
			return HTTP_STATUS_CREATED;
		}

		// Download

		static constexpr const char* data = "TestContent";

		HttpStatus arrangeSendFrom(uint32_t &size) {
			size = strlen(data);
			return HTTP_STATUS_OK;
		}

		HttpStatus readContent() {
			send(data, strlen(data));
			return HTTP_STATUS_OK;
		}

		HttpStatus contentRead() {
			return HTTP_STATUS_OK;
		}

		// Listing

		HttpStatus arrangeDirectoryListing() {
			n = 0;
			return HTTP_STATUS_MULTI_STATUS;
		}

		HttpStatus arrangeFileListing() {
			n = 0;
			return HTTP_STATUS_MULTI_STATUS;
		}

		HttpStatus generateListing(const DavProperty* prop)
		{
			if(n == 0 && !prop)
				sendChunk("foo");
			else if(n == 0 && (prop == Types::davProperties + 0))
				sendChunk("1");
			else if(n == 1 && !prop)
				sendChunk("bar");
			else if(n == 1 && (prop == Types::davProperties + 0))
				sendChunk("2");
			else if(prop == Types::davProperties + 1)
				sendChunk("awsome");

			return HTTP_STATUS_OK;
		}

		HttpStatus generateFileListing(const DavProperty* prop) {
			return generateListing(prop);
		}

		HttpStatus generateDirectoryListing(const DavProperty* prop)
		{
			return generateListing(prop);
		}


		bool stepListing() {
			if(n != 1) {
				n++;
				return true;
			} else {
				return false;
			}
		}

		HttpStatus fileListingDone() {
			return HTTP_STATUS_MULTI_STATUS;
		}

		HttpStatus directoryListingDone() {
			return HTTP_STATUS_MULTI_STATUS;
		}

		void process(const char* input) {
			reset();
			parse(input, strlen(input));
			done();
			CHECK(!isError(getStatus()));
		}
	};

	Uut uut;

	void expectChunked(std::string expected) {
		const char* data = uut.response.c_str();
		const char* exp = expected.c_str();

		enum State {cr, crLf, crLfCr, crLfCrLf, ChunkHead, ChunkBody, ChunkTrailer} state = cr;
		uint32_t chunkSize;
		for(unsigned int i=0; i<expected.length(); i++) {
			switch(state) {
			case cr:
				state = (*data == '\r') ? crLf : cr;
				break;
			case crLf:
				state = (*data == '\n') ? crLfCr : cr;
				break;
			case crLfCr:
				state = (*data == '\r') ? crLfCrLf : cr;
				break;
			case crLfCrLf:
				state = (*data == '\n') ? ChunkHead : cr;
				break;
			case ChunkTrailer:
				CHECK(*data++ == '\r');
				CHECK(*data++ == '\n');
				state = ChunkHead;
				/* no break */
			case ChunkHead:
				chunkSize = 0;
				while(*data != '\r') {
					chunkSize *= 16;
					if('0' <= *data && *data <= '9')
						chunkSize += *data - '0';
					else if('a' <= *data && *data <= 'f')
						chunkSize += *data - 'a' + 10;
					else
						FAIL("chunking error");
					data++;
				}

				data++;
				CHECK(*data == '\n');
				data++;

				if(!chunkSize) {
					CHECK(i == expected.length());
					return;
				}
				state = ChunkBody;
				/* no break */
			case ChunkBody:
				chunkSize--;
				if(!chunkSize) {
					state = ChunkTrailer;
				}
				break;
			}

			if(*exp != *data) {
				char temp[256];
				sprintf(temp, "exp: `%.16s` vs act: `%.16s`", exp, data);
				FAIL(temp);
			}

			data++;
			exp++;
		}
	}

	TEST_SETUP() {
		uut.response.clear();
	}
};

TEST(HttpLogicOutput, Get)
{
	uut.process("GET /foo/bar HTTP/1.1\r\n\r\n");
	CHECK(uut.response ==
			"HTTP/1.1 200 OK\r\n"
			"Connection: Keep-Alive\r\n"
			"Content-Length: 11\r\n"
			"\r\n"
			"TestContent");
}

TEST(HttpLogicOutput, GetKeepAlive)
{
	uut.process("GET /foo/bar HTTP/1.1\r\nConnection:keep-alive\r\n\r\n"
				"GET /foo/bar HTTP/1.1\r\nConnection: Close\r\n\r\n"
			);
	CHECK(uut.response ==
			"HTTP/1.1 200 OK\r\n"
			"Connection: Keep-Alive\r\n"
			"Content-Length: 11\r\n"
			"\r\n"
			"TestContentHTTP/1.1 200 OK\r\n"
			"Connection: Close\r\n"
			"Content-Length: 11\r\n"
			"\r\n"
			"TestContent");
}

TEST(HttpLogicOutput, Head)
{
	uut.process("HEAD /foo/bar HTTP/1.1\r\n\r\n");
	CHECK(uut.response ==
			"HTTP/1.1 200 OK\r\n"
			"Connection: Keep-Alive\r\n"
			"Content-Length: 11\r\n"
			"\r\n");
}

TEST(HttpLogicOutput, Put)
{
	uut.process("PUT /foo/bar HTTP/1.1\r\n"
			"Content-Length: 7\r\n"
			"Connection: Keep-Alive\r\n"
			"\r\n"
			"Content");

	CHECK(uut.response ==
			"HTTP/1.1 201 Created\r\n"
			"Connection: Keep-Alive\r\n"
			"Content-Length: 0\r\n"
			"\r\n");
}

TEST(HttpLogicOutput, Delete)
{
	uut.process("DELETE /foo/bar HTTP/1.1\r\n\r\n");

	CHECK(uut.response ==
			"HTTP/1.1 204 No Content\r\n"
			"Connection: Keep-Alive\r\n"
			"Content-Length: 0\r\n"
			"\r\n");
}

TEST(HttpLogicOutput, Mkdir)
{
	uut.process("MKCOL /foo/bar HTTP/1.1\r\n\r\n");

	CHECK(uut.response ==
			"HTTP/1.1 201 Created\r\n"
			"Connection: Keep-Alive\r\n"
			"Content-Length: 0\r\n"
			"\r\n");
}

TEST(HttpLogicOutput, Copy)
{
	uut.process("COPY /foo/bar HTTP/1.1\r\n"
			"Destination:/baz\r\n\r\n");

	CHECK(uut.response ==
			"HTTP/1.1 201 Created\r\n"
			"Connection: Keep-Alive\r\n"
			"Content-Length: 0\r\n"
			"\r\n");
}

TEST(HttpLogicOutput, Move)
{
	uut.process("MOVE /foo/bar HTTP/1.1\r\n"
			"Destination:/baz\r\n\r\n");

	CHECK(uut.response ==
			"HTTP/1.1 201 Created\r\n"
			"Connection: Keep-Alive\r\n"
			"Content-Length: 0\r\n"
			"\r\n");
}

TEST(HttpLogicOutput, Options)
{
	uut.process("OPTIONS /foo/bar HTTP/1.1\r\n"
			"Destination:/baz\r\n\r\n");

	CHECK(uut.response ==
			"HTTP/1.1 200 OK\r\n"
			"Connection: Keep-Alive\r\n"
			"Allow: OPTIONS,GET,PUT,HEAD,DELETE,PROPFIND,COPY,MOVE\r\n"
			"Dav: 1\r\n"
			"Content-Length: 0\r\n"
			"\r\n");
}


IGNORE_TEST(HttpLogicOutput, PropfindAllprops)
{
	uut.process("PROPFIND / HTTP/1.1\r\n"
			"Depth:1\r\n\r\n");

	expectChunked(
			"HTTP/1.1 207 Multi-Status\r\n"
			"Connection: Keep-Alive\r\n"
			"Transfer-Encoding: chunked\r\n"
			"\r\n"
			"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
			"<multistatus xmlns=\"DAV:\">"
				"<response>"
					"<href>foo</href>"
					"<propstat>"
						"<prop>"
							"<getcontentlength xmlns='DAV:'>1</getcontentlength>"
							"<otherprop xmlns='foo://bar'>awsome</otherprop>"
						"</prop>"
						"<status>HTTP/1.1 200 OK</status>"
					"</propstat>"
				"</response>"
				"<response>"
					"<href>bar</href>"
					"<propstat>"
						"<prop>"
							"<getcontentlength xmlns='DAV:'>2</getcontentlength>"
							"<otherprop xmlns='foo://bar'>awsome</otherprop>"
						"</prop>"
						"<status>HTTP/1.1 200 OK</status>"
					"</propstat>"
				"</response>"
			"</multistatus>");
}


IGNORE_TEST(HttpLogicOutput, PropfindProp)
{
	const char* body =
		"<?xml version='1.0' encoding='utf-8'?>"
		"<propfind xmlns='DAV:'><prop>"
		"<getcontentlength xmlns='DAV:'/>"
		"<getlastmodified xmlns='DAV:'/>"
		"<executable xmlns='http://apache.org/dav/props/'/>"
		"<resourcetype xmlns='DAV:'/>"
		"<checked-in xmlns='DAV:'/>"
		"<checked-out xmlns='DAV:'/>"
		"</prop></propfind>";

	static char temp[1024*1024];
	sprintf(temp,
			"PROPFIND / HTTP/1.1\r\n"
			"Depth:1\r\n"
			"Content-Length:%d\r\n"
			"\r\n"
			"%s", (int)strlen(body), body);
	uut.process(temp);

	expectChunked(
			"HTTP/1.1 207 Multi-Status\r\n"
			"Connection: Keep-Alive\r\n"
			"Transfer-Encoding: chunked\r\n"
			"\r\n"
			"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
			"<multistatus xmlns=\"DAV:\">"
				"<response>"
					"<href>foo</href>"
					"<propstat>"
						"<prop>"
							"<getcontentlength xmlns='DAV:'>1</getcontentlength>"
						"</prop>"
						"<status>HTTP/1.1 200 OK</status>"
					"</propstat>"
					"<propstat>"
						"<prop>"
							"<checked-out xmlns='DAV:'/>"
							"<checked-in xmlns='DAV:'/>"
							"<resourcetype xmlns='DAV:'/>"
							"<executable xmlns='http://apache.org/dav/props/'/>"
							"<getlastmodified xmlns='DAV:'/>"
						"</prop>"
						"<status>HTTP/1.1 404 Not Found</status>"
					"</propstat>"
				"</response>"
				"<response>"
					"<href>bar</href>"
					"<propstat>"
						"<prop>"
							"<getcontentlength xmlns='DAV:'>2</getcontentlength>"
						"</prop>"
						"<status>HTTP/1.1 200 OK</status>"
					"</propstat>"
					"<propstat>"
						"<prop>"
							"<checked-out xmlns='DAV:'/>"
							"<checked-in xmlns='DAV:'/>"
							"<resourcetype xmlns='DAV:'/>"
							"<executable xmlns='http://apache.org/dav/props/'/>"
							"<getlastmodified xmlns='DAV:'/>"
						"</prop>"
						"<status>HTTP/1.1 404 Not Found</status>"
					"</propstat>"
				"</response>"
			"</multistatus>");
}


IGNORE_TEST(HttpLogicOutput, PropfindPropnames)
{
	const char* body =
		"<?xml version='1.0' encoding='utf-8'?>"
		"<propfind xmlns='DAV:'><propname/></propfind>";

	static char temp[1024*1024];
	sprintf(temp,
			"PROPFIND / HTTP/1.1\r\n"
			"Depth:1\r\n"
			"Content-Length:%d\r\n"
			"\r\n"
			"%s", (int)strlen(body), body);
	uut.process(temp);

	expectChunked(
			"HTTP/1.1 207 Multi-Status\r\n"
			"Connection: Keep-Alive\r\n"
			"Transfer-Encoding: chunked\r\n"
			"\r\n"
			"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
			"<multistatus xmlns=\"DAV:\">"
				"<response>"
					"<href>foo</href>"
					"<propstat>"
						"<prop>"
							"<getcontentlength xmlns='DAV:'></getcontentlength>"
							"<otherprop xmlns='foo://bar'></otherprop>"
						"</prop>"
						"<status>HTTP/1.1 200 OK</status>"
					"</propstat>"
				"</response>"
				"<response>"
					"<href>bar</href>"
					"<propstat>"
						"<prop>"
							"<getcontentlength xmlns='DAV:'></getcontentlength>"
							"<otherprop xmlns='foo://bar'></otherprop>"
						"</prop>"
						"<status>HTTP/1.1 200 OK</status>"
					"</propstat>"
				"</response>"
			"</multistatus>");
}
