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

#include "CppUTestExt/MockSupport.h"

#include "DavProperty.h"

namespace {

	struct Types {
		static constexpr const char* username = "foo";
		static constexpr const char* realm = "bar";
		static constexpr const char* RFC2069_A1 = "d65f52b42a2605dd84ef29a88bd75e1d";
		static constexpr const uint32_t davStackSize = 192;

		static constexpr const DavProperty davProperties[] {
			DavProperty("DAV:", "getcontentlength")
		};

		struct ResourceLocator {
			std::string path;
		};

		typedef ResourceLocator SourceLocator;
		typedef ResourceLocator DestinationLocator;
	};

	struct MockedHttpLogic: public HttpLogic<MockedHttpLogic, Types> {
		using ResourceLocator = Types::ResourceLocator;
		using SourceLocator = Types::SourceLocator;
		using DestinationLocator = Types::DestinationLocator;

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

		void resetLocator(ResourceLocator* rl) {
			mock("ResourceLocator").actualCall("reset");
			rl->path.clear();
		}

		DavAccess accessible(ResourceLocator* rl, bool authenticated)
		{
			return DavAccess::Dav;
		}

		HttpStatus enter(ResourceLocator* rl, const char* str, unsigned int length) {
			mock("ResourceLocator").actualCall("enter").withStringParameter("str", std::string(str, length).c_str());
			rl->path += std::string("/") + std::string(str, length);
			return HTTP_STATUS_OK;
		}

		static HttpStatus createDirectory(DestinationLocator* rl, const char* dstName, uint32_t length) {
			mock("ContentProvider").actualCall("createDirectory")
			.withStringParameter("path", (rl->path + std::string("/") + std::string(dstName, length)).c_str());
			return HTTP_STATUS_OK;
		}

		static HttpStatus remove(DestinationLocator* rl, const char* dstName, uint32_t length) {
			mock("ContentProvider").actualCall("remove")
			.withStringParameter("path", (rl->path + std::string("/") + std::string(dstName, length)).c_str());
			return HTTP_STATUS_OK;
		}

		static HttpStatus copy(SourceLocator* src, DestinationLocator* dstDir, const char* dstName, uint32_t length, bool overwrite) {
			mock("ContentProvider").actualCall("copy")
			.withStringParameter("src", src->path.c_str())
			.withStringParameter("dst", (dstDir->path + std::string("/") + std::string(dstName, length)).c_str())
			.withBoolParameter("overwrite", overwrite);
			return HTTP_STATUS_OK;
		}

		static HttpStatus move(SourceLocator* src, DestinationLocator* dstDir, const char* dstName, uint32_t length, bool overwrite) {
			mock("ContentProvider").actualCall("move")
			.withStringParameter("src", src->path.c_str())
			.withStringParameter("dst", (dstDir->path + std::string("/") + std::string(dstName, length)).c_str())
			.withBoolParameter("overwrite", overwrite);
			return HTTP_STATUS_OK;
		}

		// Upload

		static HttpStatus arrangeReceiveInto(DestinationLocator* rl, const char* dstName, uint32_t length) {
			resource = rl;
			mock("ContentProvider").actualCall("receiveInto")
			.withStringParameter("path", (rl->path + std::string("/") + std::string(dstName, length)).c_str());
			temp.clear();
			content.clear();
			workerCalled = false;
			return errAt != ErrAt::OpenForWriting ? HTTP_STATUS_OK : HTTP_STATUS_FORBIDDEN;
		}

		static HttpStatus writeContent(DestinationLocator* rl, const char* buff, uint32_t length) {
			temp += std::string(buff, length);
			CHECK(resource == rl);
			workerCalled = true;
			return errAt != ErrAt::Writing  ? HTTP_STATUS_OK : HTTP_STATUS_FORBIDDEN;
		}

		static HttpStatus contentWritten(DestinationLocator* rl) {
			content = temp;
			CHECK(resource == rl);
			mock("ContentProvider").actualCall("contentWritten");
			return errAt != ErrAt::CloseForWriting ? HTTP_STATUS_OK : HTTP_STATUS_FORBIDDEN;
		}

		// Download

		static HttpStatus arrangeSendFrom(SourceLocator* rl, uint32_t &size) {
			resource = rl;
			mock("ContentProvider").actualCall("sendFrom")
			.withStringParameter("path", rl->path.c_str());
			workerCalled = false;
			return errAt != ErrAt::OpenForReading ? HTTP_STATUS_OK : HTTP_STATUS_FORBIDDEN;
		}

		static HttpStatus readContent(SourceLocator* rl) {
			CHECK(resource == rl);
			workerCalled = true;
			return errAt != ErrAt::Reading ? HTTP_STATUS_OK : HTTP_STATUS_FORBIDDEN;
		}

		static HttpStatus contentRead(SourceLocator* rl) {
			CHECK(resource == rl);
			mock("ContentProvider").actualCall("contentRead");
			return errAt != ErrAt::CloseForReading ? HTTP_STATUS_OK : HTTP_STATUS_FORBIDDEN;
		}

		// Listing

		static HttpStatus arrangeFileListing(SourceLocator* rl) {
			resource = rl;
			mock("ContentProvider").actualCall("listFile")
			.withStringParameter("path", rl->path.c_str());
			workerCalled = false;
			return errAt != ErrAt::OpenForListing ? HTTP_STATUS_OK : HTTP_STATUS_FORBIDDEN;
		}

		static HttpStatus arrangeDirectoryListing(SourceLocator* rl) {
			resource = rl;
			mock("ContentProvider").actualCall("listDirectory")
			.withStringParameter("path", rl->path.c_str());
			workerCalled = false;
			return errAt != ErrAt::OpenForListing ? HTTP_STATUS_OK : HTTP_STATUS_FORBIDDEN;
		}


		static HttpStatus generateFileListing(SourceLocator* rl, const DavProperty* prop) {
			CHECK(resource == rl);
			CHECK(!prop || prop == Types::davProperties);
			workerCalled = true;
			return errAt != ErrAt::Listing ? HTTP_STATUS_OK : HTTP_STATUS_FORBIDDEN;
		}

		static HttpStatus generateDirectoryListing(SourceLocator* rl, const DavProperty* prop) {
			CHECK(resource == rl);
			CHECK(!prop || prop == Types::davProperties);
			workerCalled = true;
			return errAt != ErrAt::Listing ? HTTP_STATUS_OK : HTTP_STATUS_FORBIDDEN;
		}

		bool stepListing(SourceLocator* rl) {
			return false;
		}

		static HttpStatus fileListingDone(SourceLocator* rl) {
			CHECK(resource == rl);
			mock("ContentProvider").actualCall("listingDone");
			return errAt != ErrAt::CloseForListing ? HTTP_STATUS_OK : HTTP_STATUS_FORBIDDEN;
		}

		static HttpStatus directoryListingDone(SourceLocator* rl) {
			CHECK(resource == rl);
			mock("ContentProvider").actualCall("listingDone");
			return errAt != ErrAt::CloseForListing ? HTTP_STATUS_OK : HTTP_STATUS_FORBIDDEN;
		}
	};

	std::string MockedHttpLogic::content, MockedHttpLogic::temp;
	MockedHttpLogic::ResourceLocator* MockedHttpLogic::resource;
	bool MockedHttpLogic::workerCalled;
	MockedHttpLogic::ErrAt MockedHttpLogic::errAt;
	constexpr const DavProperty Types::davProperties[1];
}

#endif /* MOCKEDPROVIDERS_H_ */
