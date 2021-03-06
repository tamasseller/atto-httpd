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
#ifndef HTTPLOGIC_H_
#define HTTPLOGIC_H_

#include "Keywords.h"
#include "UrlParser.h"
#include "PathParser.h"
#include "AuthDigest.h"
#include "DavRequestParser.h"
#include "HttpRequestParser.h"

#include "DavProperty.h"

#include "algorithm/Str.h"
#include "meta/Configuration.h"

namespace HttpConfig {
	PET_CONFIG_VALUE(AuthUser, const char*);
	PET_CONFIG_VALUE(AuthRealm, const char*);
	PET_CONFIG_VALUE(AuthPasswdHash, const char*);
	PET_CONFIG_VALUE(DavStackSize, uint32_t);
	PET_CONFIG_TYPE(DavProperties);
}

// TODO add checks for destination accessibility.

typedef http_status HttpStatus;

template<class Provider, class... Options>
class HttpLogic: public HttpRequestParser<HttpLogic<Provider, Options...> >,
					UrlParser<HttpLogic<Provider, Options...> >,
					PathParser<HttpLogic<Provider, Options...> >
{
public:
	enum class AuthStatus: uint8_t {
		None,
		Failed,
		Ok
	};


private:
	struct DavProps {
		typedef typename HttpConfig::DavProperties<void>::template extract<Options...>::type Input;
		template<class T> static constexpr const DavProperty* p(const int T::*) {return T::properties;}
		template<class T> static constexpr const DavProperty* p(...) {return nullptr;}
		template<class T> static constexpr size_t c(const int T::*) {return sizeof(T::properties)/sizeof(DavProperty);}
		template<class T> static constexpr size_t c(...) {return 0;}
		static constexpr const DavProperty* properties = p<Input>(0);
		static constexpr size_t count = c<Input>(0);
	};

	static constexpr uint32_t davStackSize = HttpConfig::DavStackSize<1>::extract<Options...>::value;

	struct AuthParams {
		static constexpr const char* username = HttpConfig::AuthUser<nullptr>::extract<Options...>::value;
		static constexpr const char* realm = HttpConfig::AuthRealm<nullptr>::extract<Options...>::value;
		static constexpr const char* RFC2069_A1 = HttpConfig::AuthPasswdHash<nullptr>::extract<Options...>::value;
		static constexpr const bool ok = username && realm && RFC2069_A1;
	};

	typedef void (*HeaderFieldParser)(HttpLogic*, const char*, uint32_t);
	typedef Keywords<HeaderFieldParser, 4> HeaderKeywords;
	typedef DavRequestParser<davStackSize> DavReqParser;

	static const HeaderKeywords headerKeywords;

	static constexpr const char* crLf = "\r\n";
	static constexpr const char* keepAliveHeader = "Connection: Keep-Alive\r\n";
	static constexpr const char* closeHeader = "Connection: Close\r\n";
	static constexpr const char* chunkedHeader = "Transfer-Encoding: chunked\r\n";
	static constexpr const char* emptyBodyHeader = "Content-Length: 0\r\n";
	static constexpr const char* allowStrDav = "Allow: OPTIONS,GET,PUT,HEAD,DELETE,PROPFIND,COPY,MOVE\r\n";
	static constexpr const char* davHeader = "Dav: 1\r\n";
	static constexpr const char* allowStrNoDav = "Allow: OPTIONS,GET,HEAD\r\n";

	// Once per request
	static constexpr const char* xmlFirstHeader = "<?xml version=\"1.0\" encoding=\"UTF-8\"?><multistatus xmlns=\"DAV:\">";
	static constexpr const char* xmlLastTrailer = "</multistatus>";

	// Once per file, before name
	static constexpr const char* xmlFileHeader = "<response><href>";

	// After file name, before prop values
	static constexpr const char* xmlFileKnownPropHeader = "</href><propstat><prop>";

	// After property values
	static constexpr const char* xmlFileKnownPropTrailer = "</prop><status>HTTP/1.1 200 OK</status></propstat>";

	// Before unknown props
	static constexpr const char* xmlFileUnknownPropHeader = "<propstat><prop>";

	// After unknown properties
	static constexpr const char* xmlFileUnknownPropTrailer = "</prop><status>HTTP/1.1 404 Not Found</status></propstat>";

	static constexpr const char* xmlFileTrailer = "</response>";

	enum class Depth: uint8_t {
		File, Directory, Traverse
	};

	typedef Keywords<Depth, 3> DepthKeywords;
	static const DepthKeywords depthKeywords;

	bool parseSource;
	bool overwrite;
	AuthStatus authState;
	Depth depth;

	HeaderFieldParser fieldParser;

	HttpStatus status;
	TemporaryStringBuffer<32> tempString;

	union {
		// Only used during headerName matching, result can
		// be discarded as soon as processing of value started
		typename HeaderKeywords::Matcher headerNameMatcher;

		// Only used for auth field processing, the result is copied into
		// authState property immediately in the afterHeaderValue method
		AuthDigest<AuthParams> authFieldValidator;

		// Only used for the overwrite field processing, the result is copied
		// into overwrite property immediately in the afterHeaderValue method
		ConstantStringMatcher cstrMatcher;

		// Only used for the depth field processing, the result is copied
		// into depth property immediately in the afterHeaderValue method
		typename DepthKeywords::Matcher depthMatcher;

		// Only used for WebDAV request processing, after all the data
		// originally contained in the header fields are copied
		DavReqParser davReqParser;
	};

	static void parseUsername(HttpLogic* self, const char* buff, uint32_t length);
	static void parseDepth(HttpLogic*, const char*, uint32_t);
	static void parseOverwrite(HttpLogic*, const char*, uint32_t);
	static void parseDestination(HttpLogic*, const char*, uint32_t);
	static void parseAuthorization(HttpLogic*, const char*, uint32_t);

	// UrlParser
	friend UrlParser<HttpLogic>;
	inline int onPath(const char *at, size_t length);
	inline void pathDone();
	inline void onQuery(const char* at, uint32_t length) {}
	inline void queryDone() {}

	// PathParser
	friend PathParser<HttpLogic>;
	void beforeElement();
	void parseElement(const char *at, size_t length);
	void elementDone();

	// HttpRequestParser
	friend HttpRequestParser<HttpLogic>;
	inline void beforeRequest();

	inline void beforeUrl();
	inline int onUrl(const char *at, size_t length);
	inline void afterUrl();

	inline void beforeHeaderName();
	inline int onHeaderName(const char *at, size_t length);
	inline void afterHeaderName();

	inline void beforeHeaderValue();
	inline int onHeaderValue(const char *at, size_t length);
	inline void afterHeaderValue();

	inline void afterHeaders();

	inline void beforeBody();
	inline int onBody(const char *at, size_t length);
	inline void afterBody();

	inline void afterRequest();

	/* Chunked output */
	inline void startChunk(uint32_t);
	inline void finishChunk();
	inline void sendPropStart(const DavProperty* prop);
	inline void sendPropEnd(const DavProperty* prop);

	inline void newRequest();
	inline bool generatePropfindResponse(bool file, typename DavReqParser::Type type);
protected:
	inline void beginHeaders();
	inline void sendChunk(const char*, uint32_t);
	inline void sendChunk(const char*);

	/*
	 * Default implementation of application hooks.
	 */

	inline DavAccess sourceAccessible(bool authenticated) { return DavAccess::NoDav; }
	inline DavAccess destinationAccessible(bool authenticated) { return DavAccess::NoDav; }
	inline void resetSourceLocator() {}
	inline void resetDestinationLocator() {}
	inline HttpStatus enterSource(const char* str, unsigned int length) { return HTTP_STATUS_OK; }
	inline HttpStatus enterDestination(const char* str, unsigned int length) { return HTTP_STATUS_OK; }
	inline HttpStatus remove(const char* dstName, uint32_t length) { return HTTP_STATUS_FORBIDDEN; }
	inline HttpStatus createDirectory(const char* dstName, uint32_t length) { return HTTP_STATUS_FORBIDDEN; }
	inline HttpStatus copy(const char* dstName, uint32_t length, bool overwrite) { return HTTP_STATUS_FORBIDDEN; }
	inline HttpStatus move(const char* dstName, uint32_t length, bool overwrite) { return HTTP_STATUS_FORBIDDEN; }
	inline HttpStatus arrangeReceiveInto(const char* dstName, uint32_t length) { return HTTP_STATUS_OK; }
	inline HttpStatus writeContent(const char* buff, uint32_t length) { return HTTP_STATUS_OK; }
	inline HttpStatus contentWritten() { return HTTP_STATUS_OK; }
	inline HttpStatus arrangeSendFrom(uint32_t &size) { return HTTP_STATUS_NOT_FOUND; }
	inline HttpStatus readContent() { return HTTP_STATUS_NOT_FOUND; }
	inline HttpStatus contentRead() { return HTTP_STATUS_NOT_FOUND; }
	inline HttpStatus arrangeFileListing() { return HTTP_STATUS_FORBIDDEN; }
	inline HttpStatus arrangeDirectoryListing() { return HTTP_STATUS_FORBIDDEN; }
	inline HttpStatus generateDirectoryListing(const DavProperty* prop) { return HTTP_STATUS_FORBIDDEN; }
	inline HttpStatus generateFileListing(const DavProperty* prop) { return HTTP_STATUS_FORBIDDEN; }
	inline bool stepListing() { return false; }
	inline HttpStatus fileListingDone() { return HTTP_STATUS_FORBIDDEN; }
	inline HttpStatus directoryListingDone() { return HTTP_STATUS_FORBIDDEN; }
public:
	inline AuthStatus getAuthStatus();
	inline HttpStatus getStatus();
	inline void parse(const char *at, size_t length);
	inline void reset();
	inline void done();

	static inline const char* getStatusLine(HttpStatus);
	static inline bool isError(HttpStatus);
};

template<class Provider, class... Options>
inline void HttpLogic<Provider, Options...>::
newRequest()
{
	HttpRequestParser<HttpLogic>::newRequest();

	status = HTTP_STATUS_OK;
	authState = AuthStatus::None;
	fieldParser = nullptr;
	depth = Depth::Traverse;
}

template<class Provider, class... Options>
inline void HttpLogic<Provider, Options...>::
reset()
{
	HttpRequestParser<HttpLogic>::reset();
	newRequest();
}

template<class Provider, class... Options>
inline void HttpLogic<Provider, Options...>::
done()
{
	if(HttpRequestParser<HttpLogic>::done())
		if(!isError(status))
			status = HTTP_STATUS_BAD_REQUEST;
}

template<class Provider, class... Options>
inline void HttpLogic<Provider, Options...>::
parse(const char *at, size_t length)
{
	if((size_t)HttpRequestParser<HttpLogic>::parse(at, length) != length)
		if(!isError(status))
			status = HTTP_STATUS_BAD_REQUEST;
}

template<class Provider, class... Options>
inline void HttpLogic<Provider, Options...>::
startChunk(uint32_t size)
{
	char temp[8];
	pet::Str::utoa<16>(size, temp, sizeof(temp));
	((Provider*)this)->send(temp, strlen(temp));
	((Provider*)this)->send(crLf, strlen(crLf));
}

template<class Provider, class... Options>
inline void HttpLogic<Provider, Options...>::
sendChunk(const char* str, uint32_t length)
{
	if(length) {
		startChunk(length);
		((Provider*)this)->send(str, length);
		finishChunk();
	}
}

template<class Provider, class... Options>
inline void HttpLogic<Provider, Options...>::
sendChunk(const char* str)
{
	sendChunk(str, strlen(str));
}

template<class Provider, class... Options>
inline void HttpLogic<Provider, Options...>::
sendPropStart(const DavProperty* prop)
{
	constexpr const char *preName = "<";
	constexpr const char *postName = " xmlns='";
	constexpr const char *postNs = "'>";

	startChunk(	strlen(preName)
				+ strlen(prop->name)
				+ strlen(postName)
				+ strlen(prop->xmlns)
				+ strlen(postNs));

	((Provider*)this)->send(preName, strlen(preName));
	((Provider*)this)->send(prop->name, strlen(prop->name));
	((Provider*)this)->send(postName, strlen(postName));
	((Provider*)this)->send(prop->xmlns, strlen(prop->xmlns));
	((Provider*)this)->send(postNs, strlen(postNs));

	finishChunk();
}

template<class Provider, class... Options>
inline void HttpLogic<Provider, Options...>::
sendPropEnd(const DavProperty* prop)
{
	constexpr const char *preClose = "</";
	constexpr const char *postClose = ">";

	startChunk(	strlen(preClose)
				+ strlen(prop->name)
				+ strlen(postClose));

	((Provider*)this)->send(preClose, strlen(preClose));
	((Provider*)this)->send(prop->name, strlen(prop->name));
	((Provider*)this)->send(postClose, strlen(postClose));

	finishChunk();
}

template<class Provider, class... Options>
void HttpLogic<Provider, Options...>::
finishChunk()
{
	((Provider*)this)->send(crLf, strlen(crLf));
}


template<class Provider, class... Options>
void HttpLogic<Provider, Options...>::
parseDepth(HttpLogic* self, const char* buff, uint32_t length)
{
	constexpr const char* trueStr = "T";
	if(!buff) {
		if(!length) {
			const typename DepthKeywords::Keyword* kw = self->depthMatcher.match(depthKeywords);
			if(!kw) {
				self->status = HTTP_STATUS_BAD_REQUEST;
			} else {
				self->depth = kw->getValue();
			}
		} else
			self->depthMatcher.reset();
	} else
		self->depthMatcher.progress(depthKeywords, buff, length);
}

template<class Provider, class... Options>
void HttpLogic<Provider, Options...>::
parseOverwrite(HttpLogic* self, const char* buff, uint32_t length)
{
	static constexpr const char* trueStr = "T";
	if(!buff) {
		if(!length)
			self->overwrite = self->cstrMatcher.matches(trueStr);
		else
			self->cstrMatcher.reset();
	} else
		self->cstrMatcher.progressWithMatching(trueStr, buff, length);
}

template<class Provider, class... Options>
void HttpLogic<Provider, Options...>::
parseAuthorization(HttpLogic* self, const char* buff, uint32_t length)
{
	if(!buff) {
		if(length)
			self->authFieldValidator.reset(HttpRequestParser<HttpLogic>::getMethodText(self->getMethod()));
		else {
			self->authFieldValidator.authFieldDone();
			if(!self->authFieldValidator.isAuthorized()) {
				self->status = HTTP_STATUS_UNAUTHORIZED;
				self->authState = AuthStatus::Failed;
			} else
				self->authState = AuthStatus::Ok;
		}
	} else
		self->authFieldValidator.parseAuthField(buff, length);
}

template<class Provider, class... Options>
void HttpLogic<Provider, Options...>::
parseDestination(HttpLogic* self, const char* buff, uint32_t length)
{
	if(!buff) {
		if(length) {
			self->PathParser<HttpLogic>::reset();
			self->UrlParser<HttpLogic>::reset();
			self->parseSource = false;
			((Provider*)self)->resetDestinationLocator();
			self->tempString.clear();
		} else {
			self->UrlParser<HttpLogic>::done();
		}
	} else
		self->UrlParser<HttpLogic>::parseUrl(buff, length);
}


template<class Provider, class... Options>
inline void HttpLogic<Provider, Options...>::
parseElement(const char *at, size_t length)
{
	tempString.save(at, length);
}

template<class Provider, class... Options>
inline void HttpLogic<Provider, Options...>::
beforeElement() {
	if(!parseSource	&& tempString.length())
		((Provider*)this)->enterDestination(tempString.data(), tempString.length());

	tempString.clear();
}

template<class Provider, class... Options>
inline void HttpLogic<Provider, Options...>::
elementDone() {
	if(parseSource)
		status = ((Provider*)this)->enterSource(tempString.data(), tempString.length());
}

template<class Provider, class... Options>
inline int HttpLogic<Provider, Options...>::
onPath(const char *at, size_t length)
{
	PathParser<HttpLogic>::parsePath(at, length);
	return 0;
}

template<class Provider, class... Options>
inline void HttpLogic<Provider, Options...>::
pathDone() {
	PathParser<HttpLogic>::done();
}

template<class Provider, class... Options>
inline void HttpLogic<Provider, Options...>::beforeRequest()
{
	tempString.clear();
}

template<class Provider, class... Options>
inline void HttpLogic<Provider, Options...>::beforeUrl() {
	this->UrlParser<HttpLogic>::reset();
	this->PathParser<HttpLogic>::reset();

	switch(HttpRequestParser<HttpLogic>::getMethod()) {
		case HttpRequestParser<HttpLogic>::Method::HTTP_PUT:
		case HttpRequestParser<HttpLogic>::Method::HTTP_POST:
		case HttpRequestParser<HttpLogic>::Method::HTTP_MKCOL:
		case HttpRequestParser<HttpLogic>::Method::HTTP_DELETE:
			parseSource = false;
			break;
		default:
			parseSource = true;
	}

	if(parseSource)
		((Provider*)this)->resetSourceLocator();
	else
		((Provider*)this)->resetDestinationLocator();
}

template<class Provider, class... Options>
inline int HttpLogic<Provider, Options...>::onUrl(const char *at, size_t length) {
	this->UrlParser<HttpLogic>::parseUrl(at, length);
	return 0;
}

template<class Provider, class... Options>
inline void HttpLogic<Provider, Options...>::afterUrl() {
	this->UrlParser<HttpLogic>::done();
}

template<class Provider, class... Options>
inline void HttpLogic<Provider, Options...>::beforeHeaderName() {
	headerNameMatcher.reset();
}

template<class Provider, class... Options>
inline int HttpLogic<Provider, Options...>::onHeaderName(const char *at, size_t length) {
	headerNameMatcher.progress(headerKeywords, at, length);
	return 0;
}

template<class Provider, class... Options>
inline void HttpLogic<Provider, Options...>::afterHeaderName()
{
	const typename HeaderKeywords::Keyword* kw = headerNameMatcher.match(headerKeywords);
	fieldParser = kw ? kw->getValue() : nullptr;
}

template<class Provider, class... Options>
inline void HttpLogic<Provider, Options...>::beforeHeaderValue()
{
	if(fieldParser)
		fieldParser(this, 0, -1u);
}

template<class Provider, class... Options>
inline int HttpLogic<Provider, Options...>::onHeaderValue(const char *at, size_t length)
{
	if(fieldParser)
		fieldParser(this, at, length);

	return 0;
}

template<class Provider, class... Options>
inline void HttpLogic<Provider, Options...>::afterHeaderValue()
{
	if(fieldParser)
		fieldParser(this, 0, 0);
}

template<class Provider, class... Options>
inline void HttpLogic<Provider, Options...>::afterHeaders()
{
	if(authState != AuthStatus::Failed && !isError(status)) {
		switch(HttpRequestParser<HttpLogic>::getMethod()) {
			case HttpRequestParser<HttpLogic>::Method::HTTP_PUT:
			case HttpRequestParser<HttpLogic>::Method::HTTP_POST:
				status = ((Provider*)this)->arrangeReceiveInto(tempString.data(), tempString.length());
				break;
			case HttpRequestParser<HttpLogic>::Method::HTTP_PROPFIND:
				davReqParser.reset();
				break;
			default:;
		}
	}
}

template<class Provider, class... Options>
inline void HttpLogic<Provider, Options...>::beforeBody() {}

template<class Provider, class... Options>
inline void HttpLogic<Provider, Options...>::afterBody() {}

template<class Provider, class... Options>
inline int HttpLogic<Provider, Options...>::onBody(const char *at, size_t length) {
	if(authState != AuthStatus::Failed && !isError(status)) {
		switch(HttpRequestParser<HttpLogic>::getMethod()) {
			case HttpRequestParser<HttpLogic>::Method::HTTP_PUT:
			case HttpRequestParser<HttpLogic>::Method::HTTP_POST:
				status = ((Provider*)this)->writeContent(at, length);
				break;
			case HttpRequestParser<HttpLogic>::Method::HTTP_PROPFIND:
				if(!davReqParser.parseDavRequest(at, length))
					status = HTTP_STATUS_BAD_REQUEST;
				break;
			default:;
		}
	}

	return 0;
}

template<class Provider, class... Options>
inline void HttpLogic<Provider, Options...>::beginHeaders() {
	const char *statusLine = getStatusLine(status);
	((Provider*)this)->send(statusLine, strlen(statusLine));
	if(!isError(status)) {
		if(this->shouldKeepAlive())
			((Provider*)this)->send(keepAliveHeader, strlen(keepAliveHeader));
		else
			((Provider*)this)->send(closeHeader, strlen(closeHeader));
	}
}

template<class Provider, class... Options>
inline bool HttpLogic<Provider, Options...>::generatePropfindResponse(bool file, typename DavReqParser::Type type) {
	bool error = false;
	while(!error) {
		sendChunk(xmlFileHeader);

		HttpStatus ret = file ?
				((Provider*)this)->generateFileListing(nullptr) :
				((Provider*)this)->generateDirectoryListing(nullptr);

		if(isError(ret))
			error = true;

		sendChunk(xmlFileKnownPropHeader);

		switch(type) {
			case DavReqParser::Type::Allprop:
				for(unsigned int i=0; i<DavProps::count; i++) {
					auto prop = DavProps::properties + i;
					sendPropStart(prop);

					HttpStatus ret = file ?
							((Provider*)this)->generateFileListing(prop) :
							((Provider*)this)->generateDirectoryListing(prop);

					if(isError(ret)) {
						error = true;
						break;
					}
					sendPropEnd(prop);
				}
				sendChunk(xmlFileKnownPropTrailer);
				break;
			case DavReqParser::Type::Propname:
				for(unsigned int i=0; i<DavProps::count; i++) {
					auto prop = DavProps::properties + i;
					sendPropStart(prop);
					sendPropEnd(prop);
				}
				sendChunk(xmlFileKnownPropTrailer);
				break;
			case DavReqParser::Type::Prop:
				/*
				 * Generate response entries for known properties
				 */
				for(auto it = davReqParser.propertyIterator(); it.isValid(); davReqParser.step(it)) {
					for(unsigned int i=0; i<DavProps::count; i++) {
						auto prop = DavProps::properties + i;
						const char* str;
						uint32_t len;

						it.getName(str, len);
						if(strlen(prop->name) != len || strncmp(prop->name, str, len) != 0)
							continue;

						it.getNs(str, len);
						if(strlen(prop->xmlns) != len || strncmp(prop->xmlns, str, len) != 0)
							continue;

						sendPropStart(prop);

						HttpStatus ret = file ?
								((Provider*)this)->generateFileListing(prop) :
								((Provider*)this)->generateDirectoryListing(prop);

						if(isError(ret)) {
							error = true;
							break;
						}

						sendPropEnd(prop);
					}
				}

				sendChunk(xmlFileKnownPropTrailer);

				/*
				 * Generate entries with 404 status for unknown properties
				 */

				sendChunk(xmlFileUnknownPropHeader);

				for(auto it = davReqParser.propertyIterator(); it.isValid(); davReqParser.step(it)) {
					bool found = false;

					const char *name, *ns;
					uint32_t nameLen, nsLen;

					it.getName(name, nameLen);
					it.getNs(ns, nsLen);

					for(unsigned int i=0; i<DavProps::count; i++) {
						auto prop = DavProps::properties + i;
						if(strlen(prop->name) == nameLen && strncmp(prop->name, name, nameLen) == 0) {
							if(strlen(prop->xmlns) == nsLen && strncmp(prop->xmlns, ns, nsLen) == 0) {
								found = true;
								break;
							}
						}
					}

					if(!found) {
						constexpr const char *preName = "<";
						constexpr const char *postName = " xmlns='";
						constexpr const char *postNs = "'/>";

						startChunk(	strlen(preName)
									+ nameLen
									+ strlen(postName)
									+ nsLen
									+ strlen(postNs));

						((Provider*)this)->send(preName, strlen(preName));
						((Provider*)this)->send(name, nameLen);
						((Provider*)this)->send(postName, strlen(postName));
						((Provider*)this)->send(ns, nsLen);
						((Provider*)this)->send(postNs, strlen(postNs));

						finishChunk();
					}
				}

				sendChunk(xmlFileUnknownPropTrailer);

				break;
			default:;
		}

		sendChunk(xmlFileTrailer);

		if(file)
			break;

		if(!((Provider*)this)->stepListing())
			break;

	}

	return !error;
}


template<class Provider, class... Options>
inline void HttpLogic<Provider, Options...>::afterRequest() {
	uint32_t length;

	DavAccess access = ((Provider*)this)->sourceAccessible(authState == AuthStatus::Ok);

	if(access == DavAccess::AuthNeeded && authState != AuthStatus::Ok)
		status = (authState == AuthStatus::None) ? HTTP_STATUS_UNAUTHORIZED : HTTP_STATUS_FORBIDDEN;

	if(!isError(status)) {
		switch(HttpRequestParser<HttpLogic>::getMethod()) {
			case HttpRequestParser<HttpLogic>::Method::HTTP_DELETE:
				status = ((Provider*)this)->remove(tempString.data(), tempString.length());
				break;

			case HttpRequestParser<HttpLogic>::Method::HTTP_GET:
			case HttpRequestParser<HttpLogic>::Method::HTTP_HEAD:
				status = ((Provider*)this)->arrangeSendFrom(length);
				break;

			case HttpRequestParser<HttpLogic>::Method::HTTP_PUT:
			case HttpRequestParser<HttpLogic>::Method::HTTP_POST:
				status = ((Provider*)this)->contentWritten();
				break;

			case HttpRequestParser<HttpLogic>::Method::HTTP_COPY:
				if(access == DavAccess::NoDav)
					status = HTTP_STATUS_METHOD_NOT_ALLOWED;
				else
				status = ((Provider*)this)->copy(tempString.data(), tempString.length(), overwrite);
				break;

			case HttpRequestParser<HttpLogic>::Method::HTTP_MKCOL:
				if(access == DavAccess::NoDav)
					status = HTTP_STATUS_METHOD_NOT_ALLOWED;
				else
				status = ((Provider*)this)->createDirectory(tempString.data(), tempString.length());
				break;

			case HttpRequestParser<HttpLogic>::Method::HTTP_MOVE:
				if(access == DavAccess::NoDav)
					status = HTTP_STATUS_METHOD_NOT_ALLOWED;
				else
					status = ((Provider*)this)->move(tempString.data(), tempString.length(), overwrite);
				break;

			case HttpRequestParser<HttpLogic>::Method::HTTP_PROPFIND:
				if(access == DavAccess::NoDav)
					status = HTTP_STATUS_METHOD_NOT_ALLOWED;
				else if(!davReqParser.done())
					status = HTTP_STATUS_BAD_REQUEST;
				else {
					switch(depth) {
						case Depth::File:
						case Depth::Directory:
							status = ((Provider*)this)->arrangeFileListing();
							break;
						case Depth::Traverse:
							status = HTTP_STATUS_NOT_IMPLEMENTED;
							break;
					}
				}
				break;

			case HttpRequestParser<HttpLogic>::Method::HTTP_OPTIONS:
				status = HTTP_STATUS_OK;
				break;

			default:
				status = HTTP_STATUS_METHOD_NOT_ALLOWED;
				break;
		}
	}

	beginHeaders();

	if(!isError(status)) {
		switch(HttpRequestParser<HttpLogic>::getMethod()) {
			case HttpRequestParser<HttpLogic>::Method::HTTP_GET:
			case HttpRequestParser<HttpLogic>::Method::HTTP_HEAD: {
				constexpr const char* contentLengthStr = "Content-Length: ";
				char temp[12];
				((Provider*)this)->send(contentLengthStr, strlen(contentLengthStr));
				pet::Str::utoa<10>(length, temp, sizeof(temp));
				((Provider*)this)->send(temp, strlen(temp));
				((Provider*)this)->send(crLf, strlen(crLf));
				break;
			}

			case HttpRequestParser<HttpLogic>::Method::HTTP_PROPFIND: {
				((Provider*)this)->send(chunkedHeader, strlen(chunkedHeader));
				break;
			}

			case HttpRequestParser<HttpLogic>::Method::HTTP_OPTIONS:
				if(access == DavAccess::NoDav)
					((Provider*)this)->send(allowStrNoDav, strlen(allowStrNoDav));
				else {
					((Provider*)this)->send(allowStrDav, strlen(allowStrDav));
					((Provider*)this)->send(davHeader, strlen(davHeader));
				}
				/* no break */
			default:
				((Provider*)this)->send(emptyBodyHeader, strlen(emptyBodyHeader));
		};

		((Provider*)this)->send(crLf, strlen(crLf));

		/*
		 * Generate response body
		 */
		bool error = false;
		switch(HttpRequestParser<HttpLogic>::getMethod()) {
			case HttpRequestParser<HttpLogic>::Method::HTTP_GET:
				if(isError(((Provider*)this)->readContent()))
						error = true;

			/*
			 * no break to allow fall-through for ensuring correct GET-HEAD pairing
			 */
			case HttpRequestParser<HttpLogic>::Method::HTTP_HEAD:
				if(!error && isError(((Provider*)this)->contentRead()))
						error = true;

				break;
			case HttpRequestParser<HttpLogic>::Method::HTTP_PROPFIND:
				sendChunk(xmlFirstHeader);

				if(generatePropfindResponse(true, davReqParser.getType())) {
					if(isError(((Provider*)this)->fileListingDone()))
						error = true;
					else if(depth == Depth::Directory) {
						HttpStatus ret = ((Provider*)this)->arrangeDirectoryListing();
						if(isError(ret))
							error = true;
						else {
							if(ret != HTTP_STATUS_NO_CONTENT && !generatePropfindResponse(false, davReqParser.getType()))
								error = true;

							if(isError(((Provider*)this)->directoryListingDone()))
								error = true;
						}

					}
				} else
					error = true;

				sendChunk(xmlLastTrailer);
				startChunk(0);
				finishChunk();

				break;
			default:;
		}

		if(error) {
			status = HTTP_STATUS_INTERNAL_SERVER_ERROR;
			// closeConnection();
		}
	} else {
		if(status == HTTP_STATUS_UNAUTHORIZED) {
			// TODO send WWW-Authenticate header
		}

		((Provider*)this)->send(emptyBodyHeader, strlen(emptyBodyHeader));
		((Provider*)this)->send(crLf, strlen(crLf));
	}

	((Provider*)this)->flush();

	newRequest();
}

template<class Provider, class... Options>
inline HttpStatus HttpLogic<Provider, Options...>::getStatus()
{
	return status;
}

template<class Provider, class... Options>
typename HttpLogic<Provider, Options...>::AuthStatus
inline HttpLogic<Provider, Options...>::getAuthStatus()
{
	return authState;
}

#define XX(num, name, string) case HTTP_STATUS_##name: return "HTTP/1.1 " #num " " #string "\r\n";

template<class Provider, class... Options>
inline const char* HttpLogic<Provider, Options...>::getStatusLine(HttpStatus status)
{
	switch(status) {
		HTTP_STATUS_MAP(XX)
	default:
		return 0;
	}
}

#undef XX

template<class Provider, class... Options>
inline bool HttpLogic<Provider, Options...>::isError(HttpStatus status)
{
	return status >= 400;
}

template<class Provider, class... Options>
const typename HttpLogic<Provider, Options...>::HeaderKeywords
HttpLogic<Provider, Options...>::headerKeywords({
	typename HeaderKeywords::Keyword("Depth", &HttpLogic<Provider, Options...>::parseDepth),
	typename HeaderKeywords::Keyword("Overwrite", &HttpLogic<Provider, Options...>::parseOverwrite),
	typename HeaderKeywords::Keyword("Destination", &HttpLogic<Provider, Options...>::parseDestination),
	typename HeaderKeywords::Keyword("Authorization", &HttpLogic<Provider, Options...>::parseAuthorization),
});

template<class Provider, class... Options>
const typename HttpLogic<Provider, Options...>::DepthKeywords
HttpLogic<Provider, Options...>::depthKeywords({
	typename DepthKeywords::Keyword("0", HttpLogic<Provider, Options...>::Depth::File),
	typename DepthKeywords::Keyword("1", HttpLogic<Provider, Options...>::Depth::Directory),
	typename DepthKeywords::Keyword("infinity", HttpLogic<Provider, Options...>::Depth::Traverse),
});

#endif /* HTTPLOGIC_H_ */
