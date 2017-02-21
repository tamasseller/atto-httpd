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
#ifndef HTTPREQUESTPARSER_H_
#define HTTPREQUESTPARSER_H_

#include "http-parser/http_parser.h"

template<class Child>
class HttpRequestParser: http_parser {
	static const http_parser_settings ngnixParserSettings;
	static int onNgnixMessageBegin(http_parser*);
	static int onNgnixUrl(http_parser*, const char *at, size_t length);
	static int onNgnixHeaderField(http_parser*, const char *at, size_t length);
	static int onNgnixHeaderValue(http_parser*, const char *at, size_t length);
	static int onNgnixHeadersComplete(http_parser*);
	static int onNgnixBody(http_parser*, const char *at, size_t length);
	static int onNgnixMessageComplete(http_parser*);

	enum class State: uint8_t {
		Initial, Url, HeaderName, HeaderValue, Body, Done
	};

	State hState;

protected:
	inline void beforeRequest() {}

	inline void beforeUrl() {}
	inline int onUrlPath(const char *at, size_t length) {return 0;}
	inline void afterUrl() {}

	inline void beforeHeaderName() {}
	inline int onHeaderName(const char *at, size_t length) {return 0;}
	inline void afterHeaderName() {}

	inline void beforeHeaderValue() {}
	inline int onHeaderValue(const char *at, size_t length) {return 0;}
	inline void afterHeaderValue() {}

	inline void afterHeaders() {}

	inline void beforeBody() {}
	inline int onBody(const char *at, size_t length) {return 0;}
	inline void afterBody() {}

	inline void afterRequest() {}

	inline void newRequest();
public:
	typedef http_method Method;
	inline Method getMethod() {return (Method)method;}

	static const char* getMethodText(Method);

	inline HttpRequestParser();
	inline void reset();
	inline int parse(const char *at, size_t length);
	inline int done();
};

template<class Child>
inline HttpRequestParser<Child>::HttpRequestParser(): hState(State::Initial) {}

template<class Child>
inline void HttpRequestParser<Child>::newRequest()
{
	hState = State::Initial;
}

template<class Child>
inline void HttpRequestParser<Child>::reset()
{
	newRequest();
	http_parser_init(this, HTTP_REQUEST);
}

template<class Child>
inline int HttpRequestParser<Child>::parse(const char *at, size_t length)
{
	return http_parser_execute((http_parser*)this, &ngnixParserSettings, at, length);
}

template<class Child>
inline int HttpRequestParser<Child>::done()
{
	return http_parser_execute((http_parser*)this, &ngnixParserSettings, 0, 0);
}

template<class Child>
int HttpRequestParser<Child>::onNgnixMessageBegin(http_parser* self)
{
	((Child*)(HttpRequestParser*)self)->beforeRequest();
	return 0;
}

template<class Child>
int HttpRequestParser<Child>::onNgnixUrl(http_parser* self, const char *at, size_t length)
{
	HttpRequestParser* me = (HttpRequestParser*)self;
	Child* child = (Child*) me;

	if(me->hState == State::Initial) {
		me->hState = State::Url;
		child->beforeUrl();
	}

	return child->onUrl(at, length);
}

template<class Child>
int HttpRequestParser<Child>::onNgnixHeaderField(http_parser* self, const char *at, size_t length)
{
	HttpRequestParser* me = (HttpRequestParser*)self;
	Child* child = (Child*) me;

	if(me->hState != State::HeaderName) {
		if(me->hState == State::Url)
			child->afterUrl();

		if(me->hState == State::HeaderValue)
			child->afterHeaderValue();

		me->hState = State::HeaderName;

		child->beforeHeaderName();
	}

	return child->onHeaderName(at, length);
}

template<class Child>
int HttpRequestParser<Child>::onNgnixHeaderValue(http_parser* self, const char *at, size_t length)
{
	HttpRequestParser* me = (HttpRequestParser*)self;
	Child* child = (Child*) me;

	if(me->hState != State::HeaderValue) {
		if(me->hState == State::HeaderName)
			child->afterHeaderName();

		me->hState = State::HeaderValue;

		child->beforeHeaderValue();
	}

	return child->onHeaderValue(at, length);
}

template<class Child>
int HttpRequestParser<Child>::onNgnixHeadersComplete(http_parser* self)
{
	HttpRequestParser* me = (HttpRequestParser*)self;
	Child* child = (Child*) me;

	if(me->hState == State::Url)
		child->afterUrl();

	else if(me->hState == State::HeaderValue)
		child->afterHeaderValue();

	child->afterHeaders();

	return 0;
}

template<class Child>
int HttpRequestParser<Child>::onNgnixBody(http_parser* self, const char *at, size_t length)
{
	HttpRequestParser* me = (HttpRequestParser*)self;
	Child* child = (Child*) me;

	if(me->hState != State::Body) {
		me->hState = State::Body;
		child->beforeBody();
	}

	return child->onBody(at, length);
}

template<class Child>
int HttpRequestParser<Child>::onNgnixMessageComplete(http_parser* self)
{
	HttpRequestParser* me = (HttpRequestParser*)self;
	Child* child = (Child*) me;

	if(me->hState == State::Body)
		child->afterBody();

	me->hState = State::Done;
	child->afterRequest();

	return 0;
}

#define XX(num, name, string) case HTTP_##name: return #string;

template<class Child>
const char* HttpRequestParser<Child>::getMethodText(Method method)
{
	switch(method) {
		HTTP_METHOD_MAP(XX)
	default:
		return 0;
	}
}

template<class Child>
const http_parser_settings HttpRequestParser<Child>::ngnixParserSettings = {
	&HttpRequestParser<Child>::onNgnixMessageBegin,
	&HttpRequestParser<Child>::onNgnixUrl,
	0,
	&HttpRequestParser<Child>::onNgnixHeaderField,
	&HttpRequestParser<Child>::onNgnixHeaderValue,
	&HttpRequestParser<Child>::onNgnixHeadersComplete,
	&HttpRequestParser<Child>::onNgnixBody,
	&HttpRequestParser<Child>::onNgnixMessageComplete,
	0,
	0
};

#undef XX

#endif /* HTTPREQUESTPARSER_H_ */
