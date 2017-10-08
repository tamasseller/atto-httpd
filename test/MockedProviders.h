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
#ifndef MOCKEDPROVIDERS_H_
#define MOCKEDPROVIDERS_H_

#include "1test/Mock.h"

#include "DavProperty.h"

#include <string>

namespace {

	static constexpr const char username[] = "foo";
	static constexpr const char realm[] = "bar";
	static constexpr const char RFC2069_A1[] = "d65f52b42a2605dd84ef29a88bd75e1d";

	struct DavProperties {
		static constexpr const DavProperty properties[] {
			DavProperty("DAV:", "getcontentlength")
		};
	};

	struct MockedHttpLogic: public HttpLogic<MockedHttpLogic,
		HttpConfig::AuthUser<username>,
		HttpConfig::AuthRealm<realm>,
		HttpConfig::AuthPasswdHash<RFC2069_A1>,
		HttpConfig::DavStackSize<192>,
		HttpConfig::DavProperties<DavProperties>
	> {
		struct ResourceLocator {
			std::string path;
		} src, dst;

		enum ErrAt {
			None,
			OpenForListing,
			OpenForReading,
			OpenForWriting,
			Listing,
			Reading,
			Writing,
			CloseForListing,
			CloseForReading,
			CloseForWriting
		};

		static std::string content, temp;
		static bool workerCalled;
		static ResourceLocator* resource;
		static ErrAt errAt;

		void send(const char* str, unsigned int length) {}
		void flush() {}

		void resetSourceLocator() {
			MOCK(ResourceLocator)::CALL(reset);
			src.path.clear();
		}

		void resetDestinationLocator() {
			MOCK(ResourceLocator)::CALL(reset);
			dst.path.clear();
		}

		DavAccess sourceAccessible(bool authenticated)
		{
			return DavAccess::Dav;
		}

		HttpStatus enterSource(const char* str, unsigned int length) {
			MOCK(ResourceLocator)::CALL(enter).withStringParam(std::string(str, length).c_str());
			src.path += std::string("/") + std::string(str, length);
			return HTTP_STATUS_OK;
		}

		HttpStatus enterDestination(const char* str, unsigned int length) {
			MOCK(ResourceLocator)::CALL(enter).withStringParam(std::string(str, length).c_str());
			dst.path += std::string("/") + std::string(str, length);
			return HTTP_STATUS_OK;
		}

		HttpStatus createDirectory(const char* dstName, uint32_t length) {
			MOCK(ContentProvider)::CALL(createDirectory)
			.withStringParam((dst.path + std::string("/") + std::string(dstName, length)).c_str());
			return HTTP_STATUS_OK;
		}

		HttpStatus remove(const char* dstName, uint32_t length) {
			MOCK(ContentProvider)::CALL(remove)
			.withStringParam((dst.path + std::string("/") + std::string(dstName, length)).c_str());
			return HTTP_STATUS_OK;
		}

		HttpStatus copy(const char* dstName, uint32_t length, bool overwrite) {
			MOCK(ContentProvider)::CALL(copy)
			.withStringParam(src.path.c_str())
			.withStringParam((dst.path + std::string("/") + std::string(dstName, length)).c_str())
			.withParam(overwrite);
			return HTTP_STATUS_OK;
		}

		HttpStatus move(const char* dstName, uint32_t length, bool overwrite) {
			MOCK(ContentProvider)::CALL(move)
			.withStringParam(src.path.c_str())
			.withStringParam((dst.path + std::string("/") + std::string(dstName, length)).c_str())
			.withParam(overwrite);
			return HTTP_STATUS_OK;
		}

		// Upload

		HttpStatus arrangeReceiveInto(const char* dstName, uint32_t length) {
			resource = &dst;
			MOCK(ContentProvider)::CALL(receiveInto)
			.withStringParam((dst.path + std::string("/") + std::string(dstName, length)).c_str());
			temp.clear();
			content.clear();
			workerCalled = false;
			return errAt != ErrAt::OpenForWriting ? HTTP_STATUS_OK : HTTP_STATUS_FORBIDDEN;
		}

		HttpStatus writeContent(const char* buff, uint32_t length) {
			temp += std::string(buff, length);
			CHECK(resource == &dst);
			workerCalled = true;
			return errAt != ErrAt::Writing  ? HTTP_STATUS_OK : HTTP_STATUS_FORBIDDEN;
		}

		HttpStatus contentWritten() {
			content = temp;
			CHECK(resource == &dst);
			MOCK(ContentProvider)::CALL(contentWritten);
			return errAt != ErrAt::CloseForWriting ? HTTP_STATUS_OK : HTTP_STATUS_FORBIDDEN;
		}

		// Download

		HttpStatus arrangeSendFrom(uint32_t &size) {
			resource = &src;
			MOCK(ContentProvider)::CALL(sendFrom)
			.withStringParam(src.path.c_str());
			workerCalled = false;
			return errAt != ErrAt::OpenForReading ? HTTP_STATUS_OK : HTTP_STATUS_FORBIDDEN;
		}

		HttpStatus readContent() {
			CHECK(resource == &src);
			workerCalled = true;
			return errAt != ErrAt::Reading ? HTTP_STATUS_OK : HTTP_STATUS_FORBIDDEN;
		}

		HttpStatus contentRead() {
			CHECK(resource == &src);
			MOCK(ContentProvider)::CALL(contentRead);
			return errAt != ErrAt::CloseForReading ? HTTP_STATUS_OK : HTTP_STATUS_FORBIDDEN;
		}

		// Listing

		HttpStatus arrangeFileListing() {
			resource = &src;
			MOCK(ContentProvider)::CALL(listFile)
			.withStringParam(src.path.c_str());
			workerCalled = false;
			return errAt != ErrAt::OpenForListing ? HTTP_STATUS_OK : HTTP_STATUS_FORBIDDEN;
		}

		HttpStatus arrangeDirectoryListing() {
			resource = &src;
			MOCK(ContentProvider)::CALL(listDirectory)
			.withStringParam(src.path.c_str());
			workerCalled = false;
			return errAt != ErrAt::OpenForListing ? HTTP_STATUS_OK : HTTP_STATUS_FORBIDDEN;
		}

		HttpStatus generateFileListing(const DavProperty* prop) {
			CHECK(resource == &src);
			CHECK(!prop || prop == DavProperties::properties);
			workerCalled = true;
			return errAt != ErrAt::Listing ? HTTP_STATUS_OK : HTTP_STATUS_FORBIDDEN;
		}

		HttpStatus generateDirectoryListing(const DavProperty* prop) {
			CHECK(resource == &src);
			CHECK(!prop || prop == DavProperties::properties);
			workerCalled = true;
			return errAt != ErrAt::Listing ? HTTP_STATUS_OK : HTTP_STATUS_FORBIDDEN;
		}

		bool stepListing() {
			return false;
		}

		HttpStatus fileListingDone() {
			CHECK(resource == &src);
			MOCK(ContentProvider)::CALL(listingDone);
			return errAt != ErrAt::CloseForListing ? HTTP_STATUS_OK : HTTP_STATUS_FORBIDDEN;
		}

		HttpStatus directoryListingDone() {
			CHECK(resource == &src);
			MOCK(ContentProvider)::CALL(listingDone);
			return errAt != ErrAt::CloseForListing ? HTTP_STATUS_OK : HTTP_STATUS_FORBIDDEN;
		}
	};

	std::string MockedHttpLogic::content, MockedHttpLogic::temp;
	MockedHttpLogic::ResourceLocator* MockedHttpLogic::resource;
	bool MockedHttpLogic::workerCalled;
	MockedHttpLogic::ErrAt MockedHttpLogic::errAt;
	constexpr const DavProperty DavProperties::properties[1];
}

#endif /* MOCKEDPROVIDERS_H_ */
