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
#ifndef AUTHDIGEST_H_
#define AUTHDIGEST_H_

#include "KvParser.h"
#include "Keywords.h"
#include "HexParser.h"
#include "SplitParser.h"
#include "ConstantStringMatcher.h"
#include "TemporaryStringBuffer.h"

#include "md5/md5.h"

template<class AuthProvider>
class AuthDigest: public 	Splitter<AuthDigest<AuthProvider> >,
							KvParser<AuthDigest<AuthProvider> > {
	friend Splitter<AuthDigest>;
	friend KvParser<AuthDigest>;

	typedef void (*AuthFieldParser)(AuthDigest*, const char*, uint32_t);
	typedef Keywords<AuthFieldParser, 7> AuthKeywords;
	static const AuthKeywords authKeywords;
	AuthFieldParser fieldParser;

	struct {
		MD5_CTX md5Context;
		unsigned char A2[16];
		HexParser<16> response;
		TemporaryStringBuffer<16> nonceHolder;
		ConstantStringMatcher cstrMatcher;
	} RFC2069;

	enum class State {
		Initial,
		AuthTypeWrong,
		AuthTypeOk,
		AuthFailed,
		AuthSucces
	};

	static void parseUsername(AuthDigest* self, const char* buff, uint32_t length) {
		if(!buff) {
			if(length)
				self->RFC2069.cstrMatcher.reset();
			else if(!self->RFC2069.cstrMatcher.matches(AuthProvider::username))
				self->state = State::AuthFailed;
		} else {
			self->RFC2069.cstrMatcher.progressWithMatching(AuthProvider::username, buff, length);
		}
	}

	static void parseNonce(AuthDigest* self, const char* buff, uint32_t length)
	{
		if(!buff) {
			if(length)
				self->RFC2069.nonceHolder.clear();
		} else {
			self->RFC2069.nonceHolder.save(buff, length);
		}
	}

	static void parseUri(AuthDigest* self, const char* buff, uint32_t length)
	{
		if(!buff) {
			if(!length)
				MD5_Final(self->RFC2069.A2, &self->RFC2069.md5Context);
		} else {
			MD5_Update(&self->RFC2069.md5Context, buff, length);
		}
	}

	static void parseRealm(AuthDigest* self, const char* buff, uint32_t length)
	{
		if(!buff) {
			if(length)
				self->RFC2069.cstrMatcher.reset();
			else if(!self->RFC2069.cstrMatcher.matches(AuthProvider::realm))
				self->state = State::AuthFailed;
		} else {
			self->RFC2069.cstrMatcher.progressWithMatching(AuthProvider::realm, buff, length);
		}
	}

	static void parseResponse(AuthDigest* self, const char* buff, uint32_t length)
	{
		if(!buff) {
			if(length)
				self->RFC2069.response.clear();
			else if(!self->RFC2069.response.isDone())
				self->state = State::AuthFailed;
		} else {
			self->RFC2069.response.parseHex(buff, length);
		}
	}

	static void parseAlgorithm(AuthDigest* self, const char* buff, uint32_t length)
	{
		if(!buff) {
			if(length)
				self->RFC2069.cstrMatcher.reset();
			else if(!self->RFC2069.cstrMatcher.matches("MD5"))
				self->state = State::AuthFailed;
		} else {
			self->RFC2069.cstrMatcher.progressWithMatching("MD5", buff, length);
		}
	}

	State state;
	typename AuthKeywords::Matcher keywordMatcher;

	void parseKey(const char* buff, unsigned int length)
	{
		if(state != State::AuthTypeWrong)
			keywordMatcher.progress(authKeywords, buff, length);
	}

	void keyDone()
	{
		const typename AuthKeywords::Keyword *kw = keywordMatcher.match(authKeywords);
		keywordMatcher.reset();

		if(!kw) {
			fieldParser = nullptr;

			if(state != State::AuthTypeOk)
				state = State::AuthTypeWrong;

		} else {
			fieldParser = kw->getValue();

			if(state != State::AuthTypeOk)
				state = (!fieldParser) ? State::AuthTypeOk : State::AuthTypeWrong;
			else if(fieldParser) {
				fieldParser(this, nullptr, -1u);
			}
		}
	}

	void parseValue(const char* buff, unsigned int length)
	{
		if(state == State::AuthTypeOk && fieldParser)
			fieldParser(this, buff, length);
	}

	void valueDone()
	{
		if(state == State::AuthTypeOk && fieldParser) {
			fieldParser(this, nullptr, 0);
			fieldParser = nullptr;
		}
	}

	// Splitter
	void parseField(const char* buff, unsigned int length)
	{
		KvParser<AuthDigest>::progressWithKv(buff, length);
	}

	// Splitter
	void fieldDone()
	{
		KvParser<AuthDigest>::kvDone();
		KvParser<AuthDigest>::reset();
	}

public:
	inline void parseAuthField(const char* buff, unsigned int length) {
		Splitter<AuthDigest>::progressWithSplitting(buff, length);
	}

	inline void authFieldDone()
	{
		Splitter<AuthDigest>::splittingDone();
		const char* hashA1 = AuthProvider::RFC2069_A1;

		if(state == State::AuthTypeOk && hashA1) {
			MD5_Init(&RFC2069.md5Context);
			MD5_Update(&RFC2069.md5Context, hashA1, strlen(hashA1));
			MD5_Update(&RFC2069.md5Context, ":", 1);
			MD5_Update(&RFC2069.md5Context, RFC2069.nonceHolder.data(), RFC2069.nonceHolder.length());
			MD5_Update(&RFC2069.md5Context, ":", 1);

			unsigned char str[16];

			for(int i=0; i<2; i++) {
				for(int j=0; j<8; j++) {
					static const char hex[] = "0123456789abcdef";
					str[2*j + 0] = hex[RFC2069.A2[8 * i + j] >> 4];
					str[2*j + 1] = hex[RFC2069.A2[8 * i + j] & 0xf];
				}
				MD5_Update(&RFC2069.md5Context, str, sizeof(str));
			}

			MD5_Final(str, &RFC2069.md5Context);

			if(memcmp(str, RFC2069.response.data(), sizeof(str)) == 0) {
				state = State::AuthSucces;
			} else
				state = State::AuthFailed;
		}
	}

	void reset(const char* method) {
		MD5_Init(&RFC2069.md5Context);
		MD5_Update(&RFC2069.md5Context, method, strlen(method));
		MD5_Update(&RFC2069.md5Context, ":", 1);
		Splitter<AuthDigest>::reset();
		KvParser<AuthDigest>::reset();
    	keywordMatcher.reset();
    	state = State::Initial;
	}

	bool isAuthorized() {
		return state == State::AuthSucces;
	}
};

template<class AuthProvider>
const typename AuthDigest<AuthProvider>::AuthKeywords
AuthDigest<AuthProvider>::authKeywords({
	typename AuthKeywords::Keyword("uri", &AuthDigest<AuthProvider>::parseUri),
	typename AuthKeywords::Keyword("nonce", &AuthDigest<AuthProvider>::parseNonce),
	typename AuthKeywords::Keyword("realm", &AuthDigest<AuthProvider>::parseRealm),
	typename AuthKeywords::Keyword("Digest", nullptr),
	typename AuthKeywords::Keyword("response", &AuthDigest<AuthProvider>::parseResponse),
	typename AuthKeywords::Keyword("username", &AuthDigest<AuthProvider>::parseUsername),
	typename AuthKeywords::Keyword("algorithm", &AuthDigest<AuthProvider>::parseAlgorithm)

	/*
		For RFC2617 and RFC7235

		AuthKeywords::Keyword("cnonce", AuthField::CNonce),
		AuthKeywords::Keyword("nc", AuthField::Nc),
		AuthKeywords::Keyword("qop", AuthField::Qop)
	*/
});



#endif /* AUTHDIGEST_H_ */
