/*
 * HttpdLogic.h
 *
 *  Created on: 2017.02.09.
 *      Author: tooma
 */

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

typedef http_status HttpStatus;

template<class Provider, class Resources>
class HttpLogic: public HttpRequestParser<HttpLogic<Provider, Resources> >, // 40
					UrlParser<HttpLogic<Provider, Resources> >, // 1
					PathParser<HttpLogic<Provider, Resources> > // 1
{
public:
	enum class AuthStatus: uint8_t {
		None,
		Failed,
		Ok
	};

private:
	typedef void (*HeaderFieldParser)(HttpLogic*, const char*, uint32_t);
	typedef Keywords<HeaderFieldParser, 4> HeaderKeywords;
	typedef DavRequestParser<Resources::davStackSize> DavReqParser;
	static const HeaderKeywords headerKeywords;

	const char* crLf = "\r\n";
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

	bool parseSource; // 1
	bool overwrite; // 1
	AuthStatus authState; // 1
	Depth depth; // 1

	HeaderFieldParser fieldParser; // 8
	typename Resources::ResourceLocator sourceResource, destinationResource; // 64

	HttpStatus status; // 4
	TemporaryStringBuffer<32> tempString; // 36

	union {
		// Only used during headerName matching, result can
		// be discarded as soon as processing of value started
		typename HeaderKeywords::Matcher headerNameMatcher;

		// Only used for auth field processing, the result is copied into
		// authState property immediately in the afterHeaderValue method
		AuthDigest<Resources> authFieldValidator; // 176 bytes

		// Only used for the overwrite field processing, the result is copied
		// into overwrite property immediately in the afterHeaderValue method
		ConstantStringMatcher cstrMatcher;

		// Only used for the depth field processing, the result is copied
		// into depth property immediately in the afterHeaderValue method
		typename DepthKeywords::Matcher depthMatcher;

		// Only used for WebDac request processing, after all the data
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
protected:
	inline void sendChunk(const char*);
public:
	inline AuthStatus getAuthStatus();
	inline HttpStatus getStatus();
	inline void parse(const char *at, size_t length);
	inline void reset();
	inline void done();

	static inline const char* getStatusLine(HttpStatus);
	static inline bool isError(HttpStatus);
};

template<class Provider, class Resources>
inline void HttpLogic<Provider, Resources>::
newRequest()
{
	HttpRequestParser<HttpLogic>::newRequest();

	status = HTTP_STATUS_OK;
	authState = AuthStatus::None;
	fieldParser = nullptr;
	depth = Depth::Traverse;
}

template<class Provider, class Resources>
inline void HttpLogic<Provider, Resources>::
reset()
{
	HttpRequestParser<HttpLogic>::reset();
	newRequest();
}

template<class Provider, class Resources>
inline void HttpLogic<Provider, Resources>::
done()
{
	if(HttpRequestParser<HttpLogic>::done())
		if(!isError(status))
			status = HTTP_STATUS_BAD_REQUEST;
}

template<class Provider, class Resources>
inline void HttpLogic<Provider, Resources>::
parse(const char *at, size_t length)
{
	if((size_t)HttpRequestParser<HttpLogic>::parse(at, length) != length)
		if(!isError(status))
			status = HTTP_STATUS_BAD_REQUEST;
}

template<class Provider, class Resources>
inline void HttpLogic<Provider, Resources>::
startChunk(uint32_t size)
{
	char temp[8];
	algorithm::Str::utoa<16>(size, temp, sizeof(temp));
	((Provider*)this)->send(temp, strlen(temp));
	((Provider*)this)->send(crLf, strlen(crLf));
}

template<class Provider, class Resources>
inline void HttpLogic<Provider, Resources>::
sendChunk(const char* str)
{
	startChunk(strlen(str));
	((Provider*)this)->send(str, strlen(str));
	finishChunk();
}

template<class Provider, class Resources>
inline void HttpLogic<Provider, Resources>::
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

template<class Provider, class Resources>
inline void HttpLogic<Provider, Resources>::
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

template<class Provider, class Resources>
void HttpLogic<Provider, Resources>::
finishChunk()
{
	((Provider*)this)->send(crLf, strlen(crLf));
}


template<class Provider, class Resources>
void HttpLogic<Provider, Resources>::
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

template<class Provider, class Resources>
void HttpLogic<Provider, Resources>::
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

template<class Provider, class Resources>
void HttpLogic<Provider, Resources>::
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

template<class Provider, class Resources>
void HttpLogic<Provider, Resources>::
parseDestination(HttpLogic* self, const char* buff, uint32_t length)
{
	if(!buff) {
		if(length) {
			self->PathParser<HttpLogic>::reset();
			self->UrlParser<HttpLogic>::reset();
			self->parseSource = false;
			((Provider*)self)->resetLocator(&self->destinationResource);
			self->tempString.clear();
		} else {
			self->UrlParser<HttpLogic>::done();
		}
	} else
		self->UrlParser<HttpLogic>::parseUrl(buff, length);
}


template<class Provider, class Resources>
inline void HttpLogic<Provider, Resources>::
parseElement(const char *at, size_t length)
{
	tempString.save(at, length);
}

template<class Provider, class Resources>
inline void HttpLogic<Provider, Resources>::
beforeElement() {
	if(!parseSource	&& tempString.length())
		((Provider*)this)->enter(&destinationResource, tempString.data(), tempString.length());

	tempString.clear();
}

template<class Provider, class Resources>
inline void HttpLogic<Provider, Resources>::
elementDone() {
	if(parseSource)
		status = ((Provider*)this)->enter(&sourceResource, tempString.data(), tempString.length());
}

template<class Provider, class Resources>
inline int HttpLogic<Provider, Resources>::
onPath(const char *at, size_t length)
{
	PathParser<HttpLogic>::parsePath(at, length);
	return 0;
}

template<class Provider, class Resources>
inline void HttpLogic<Provider, Resources>::
pathDone() {
	PathParser<HttpLogic>::done();
}

template<class Provider, class Resources>
inline void HttpLogic<Provider, Resources>::beforeRequest()
{
	tempString.clear();
}

template<class Provider, class Resources>
inline void HttpLogic<Provider, Resources>::beforeUrl() {
	this->UrlParser<HttpLogic>::reset();
	this->PathParser<HttpLogic>::reset();

	switch(HttpRequestParser<HttpLogic>::getMethod()) {
		case HttpRequestParser<HttpLogic>::Method::HTTP_PUT:
		case HttpRequestParser<HttpLogic>::Method::HTTP_MKCOL:
		case HttpRequestParser<HttpLogic>::Method::HTTP_DELETE:
			parseSource = false;
			break;
		default:
			parseSource = true;
	}

	if(parseSource)
		((Provider*)this)->resetLocator(&sourceResource);
	else
		((Provider*)this)->resetLocator(&destinationResource);
}

template<class Provider, class Resources>
inline int HttpLogic<Provider, Resources>::onUrl(const char *at, size_t length) {
	this->UrlParser<HttpLogic>::parseUrl(at, length);
	return 0;
}

template<class Provider, class Resources>
inline void HttpLogic<Provider, Resources>::afterUrl() {
	this->UrlParser<HttpLogic>::done();
}

template<class Provider, class Resources>
inline void HttpLogic<Provider, Resources>::beforeHeaderName() {
	headerNameMatcher.reset();
}

template<class Provider, class Resources>
inline int HttpLogic<Provider, Resources>::onHeaderName(const char *at, size_t length) {
	headerNameMatcher.progress(headerKeywords, at, length);
	return 0;
}

template<class Provider, class Resources>
inline void HttpLogic<Provider, Resources>::afterHeaderName()
{
	const typename HeaderKeywords::Keyword* kw = headerNameMatcher.match(headerKeywords);
	fieldParser = kw ? kw->getValue() : nullptr;
}

template<class Provider, class Resources>
inline void HttpLogic<Provider, Resources>::beforeHeaderValue()
{
	if(fieldParser)
		fieldParser(this, 0, -1u);
}

template<class Provider, class Resources>
inline int HttpLogic<Provider, Resources>::onHeaderValue(const char *at, size_t length)
{
	if(fieldParser)
		fieldParser(this, at, length);

	return 0;
}

template<class Provider, class Resources>
inline void HttpLogic<Provider, Resources>::afterHeaderValue()
{
	if(fieldParser)
		fieldParser(this, 0, 0);
}

template<class Provider, class Resources>
inline void HttpLogic<Provider, Resources>::afterHeaders()
{
	if(authState != AuthStatus::Failed && !isError(status)) {
		switch(HttpRequestParser<HttpLogic>::getMethod()) {
			case HttpRequestParser<HttpLogic>::Method::HTTP_PUT:
				status = ((Provider*)this)->arrangeReceiveInto(&destinationResource, tempString.data(), tempString.length());
				break;
			case HttpRequestParser<HttpLogic>::Method::HTTP_PROPFIND:
				davReqParser.reset();
				break;
			default:;
		}
	}
}

template<class Provider, class Resources>
inline void HttpLogic<Provider, Resources>::beforeBody() {}

template<class Provider, class Resources>
inline void HttpLogic<Provider, Resources>::afterBody() {}

template<class Provider, class Resources>
inline int HttpLogic<Provider, Resources>::onBody(const char *at, size_t length) {
	if(authState != AuthStatus::Failed && !isError(status)) {
		switch(HttpRequestParser<HttpLogic>::getMethod()) {
			case HttpRequestParser<HttpLogic>::Method::HTTP_PUT:
				status = ((Provider*)this)->writeContent(&destinationResource, at, length);
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


template<class Provider, class Resources>
inline void HttpLogic<Provider, Resources>::afterRequest() {
	uint32_t length;

	DavAccess access = ((Provider*)this)->accessible(&sourceResource, authState == AuthStatus::Ok);

	if(access == DavAccess::AuthNeeded && authState != AuthStatus::Ok)
		status = (authState == AuthStatus::None) ? HTTP_STATUS_UNAUTHORIZED : HTTP_STATUS_FORBIDDEN;

	if(!isError(status)) {
		switch(HttpRequestParser<HttpLogic>::getMethod()) {
			case HttpRequestParser<HttpLogic>::Method::HTTP_DELETE:
				status = ((Provider*)this)->remove(&destinationResource, tempString.data(), tempString.length());
				break;

			case HttpRequestParser<HttpLogic>::Method::HTTP_GET:
			case HttpRequestParser<HttpLogic>::Method::HTTP_HEAD:
				status = ((Provider*)this)->arrangeSendFrom(&sourceResource, length);
				break;

			case HttpRequestParser<HttpLogic>::Method::HTTP_PUT:
				status = ((Provider*)this)->contentWritten(&destinationResource);
				break;

			case HttpRequestParser<HttpLogic>::Method::HTTP_COPY:
				status = ((Provider*)this)->copy(&sourceResource, &destinationResource, tempString.data(), tempString.length(), overwrite);
				break;

			case HttpRequestParser<HttpLogic>::Method::HTTP_MKCOL:
				status = ((Provider*)this)->createDirectory(&destinationResource, tempString.data(), tempString.length());
				break;

			case HttpRequestParser<HttpLogic>::Method::HTTP_MOVE:
				status = ((Provider*)this)->move(&sourceResource, &destinationResource, tempString.data(), tempString.length(), overwrite);
				break;

			case HttpRequestParser<HttpLogic>::Method::HTTP_PROPFIND:
				if(!davReqParser.done())
					status = HTTP_STATUS_BAD_REQUEST;
				else {
					switch(depth) {
						case Depth::File:
							status = ((Provider*)this)->arrangeListing(&sourceResource, false);
							break;
						case Depth::Directory:
							status = ((Provider*)this)->arrangeListing(&sourceResource, true);
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

	/*
	 * Generate response header
	 */
	const char *statusLine = getStatusLine(status);
	((Provider*)this)->send(statusLine, strlen(statusLine));

	if(!isError(status)) {
		switch(HttpRequestParser<HttpLogic>::getMethod()) {
			case HttpRequestParser<HttpLogic>::Method::HTTP_GET:
			case HttpRequestParser<HttpLogic>::Method::HTTP_HEAD: {
				constexpr const char* contentLengthStr = "Content-Length: ";
				char temp[12];
				((Provider*)this)->send(contentLengthStr, strlen(contentLengthStr));
				algorithm::Str::utoa<10>(length, temp, sizeof(temp));
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
				if(isError(((Provider*)this)->readContent(&sourceResource)))
						error = true;

			/*
			 * no break to allow fall-through for ensuring correct GET-HEAD pairing
			 */
			case HttpRequestParser<HttpLogic>::Method::HTTP_HEAD:
				if(!error && isError(((Provider*)this)->contentRead(&sourceResource)))
						error = true;

				break;
			case HttpRequestParser<HttpLogic>::Method::HTTP_PROPFIND:
				sendChunk(xmlFirstHeader);
				while(!error) {
					sendChunk(xmlFileHeader);

					if(isError(((Provider*)this)->generateListing(&sourceResource, nullptr))) {
						error = true;
					}

					sendChunk(xmlFileKnownPropHeader);

					switch(davReqParser.getType()) {
						case DavReqParser::Type::Allprop:
							for(unsigned int i=0; i<sizeof(Resources::davProperties)/sizeof(Resources::davProperties[0]); i++) {
								auto prop = Resources::davProperties + i;
								sendPropStart(prop);
								if(isError(((Provider*)this)->generateListing(&sourceResource, prop))) {
									error = true;
									break;
								}
								sendPropEnd(prop);
							}
							sendChunk(xmlFileKnownPropTrailer);
							break;
						case DavReqParser::Type::Propname:
							for(unsigned int i=0; i<sizeof(Resources::davProperties)/sizeof(Resources::davProperties[0]); i++) {
								auto prop = Resources::davProperties + i;
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
								for(unsigned int i=0; i<sizeof(Resources::davProperties)/sizeof(Resources::davProperties[0]); i++) {
									auto prop = Resources::davProperties + i;
									const char* str;
									uint32_t len;

									it.getName(str, len);
									if(strlen(prop->name) != len || strncmp(prop->name, str, len) != 0)
										continue;

									it.getNs(str, len);
									if(strlen(prop->xmlns) != len || strncmp(prop->xmlns, str, len) != 0)
										continue;

									sendPropStart(prop);
									if(isError(((Provider*)this)->generateListing(&sourceResource, prop))) {
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

								for(unsigned int i=0; i<sizeof(Resources::davProperties)/sizeof(Resources::davProperties[0]); i++) {
									auto prop = Resources::davProperties + i;
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

					if(depth == Depth::File)
						break;

					if(!((Provider*)this)->stepListing(&sourceResource))
						break;

				}

				sendChunk(xmlLastTrailer);
				startChunk(0);
				finishChunk();

				if(!error && isError(((Provider*)this)->listingDone(&sourceResource)))
					error = true;

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

	newRequest();
}

template<class Provider, class Resources>
inline HttpStatus HttpLogic<Provider, Resources>::getStatus()
{
	return status;
}

template<class Provider, class Resources>
typename HttpLogic<Provider, Resources>::AuthStatus
inline HttpLogic<Provider, Resources>::getAuthStatus()
{
	return authState;
}

#define XX(num, name, string) case HTTP_STATUS_##name: return "HTTP/1.1 " #num " " #string "\r\n";

template<class Provider, class Resources>
inline const char* HttpLogic<Provider, Resources>::getStatusLine(HttpStatus status)
{
	switch(status) {
		HTTP_STATUS_MAP(XX)
	default:
		return 0;
	}
}

#undef XX

template<class Provider, class Resources>
inline bool HttpLogic<Provider, Resources>::isError(HttpStatus status)
{
	return status >= 400;
}

template<class Provider, class Resources>
const typename HttpLogic<Provider, Resources>::HeaderKeywords
HttpLogic<Provider, Resources>::headerKeywords({
	typename HeaderKeywords::Keyword("Depth", &HttpLogic<Provider, Resources>::parseDepth),
	typename HeaderKeywords::Keyword("Overwrite", &HttpLogic<Provider, Resources>::parseOverwrite),
	typename HeaderKeywords::Keyword("Destination", &HttpLogic<Provider, Resources>::parseDestination),
	typename HeaderKeywords::Keyword("Authorization", &HttpLogic<Provider, Resources>::parseAuthorization),
});

template<class Provider, class Resources>
const typename HttpLogic<Provider, Resources>::DepthKeywords
HttpLogic<Provider, Resources>::depthKeywords({
	typename DepthKeywords::Keyword("0", HttpLogic<Provider, Resources>::Depth::File),
	typename DepthKeywords::Keyword("1", HttpLogic<Provider, Resources>::Depth::Directory),
	typename DepthKeywords::Keyword("infinity", HttpLogic<Provider, Resources>::Depth::Traverse),
});


#endif /* HTTPLOGIC_H_ */
