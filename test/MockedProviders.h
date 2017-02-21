/*
 * MockedProviders.h
 *
 *  Created on: 2017.02.12.
 *      Author: tooma
 */

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
	};

	struct MockedHttpLogic: public HttpLogic<MockedHttpLogic, Types> {
		using ResourceLocator = Types::ResourceLocator;

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

		static HttpStatus createDirectory(ResourceLocator* rl, const char* dstName, uint32_t length) {
			mock("ContentProvider").actualCall("createDirectory")
			.withStringParameter("path", (rl->path + std::string("/") + std::string(dstName, length)).c_str());
			return HTTP_STATUS_OK;
		}

		static HttpStatus remove(ResourceLocator* rl, const char* dstName, uint32_t length) {
			mock("ContentProvider").actualCall("remove")
			.withStringParameter("path", (rl->path + std::string("/") + std::string(dstName, length)).c_str());
			return HTTP_STATUS_OK;
		}

		static HttpStatus copy(ResourceLocator* src, ResourceLocator* dstDir, const char* dstName, uint32_t length, bool overwrite) {
			mock("ContentProvider").actualCall("copy")
			.withStringParameter("src", src->path.c_str())
			.withStringParameter("dst", (dstDir->path + std::string("/") + std::string(dstName, length)).c_str())
			.withBoolParameter("overwrite", overwrite);
			return HTTP_STATUS_OK;
		}

		static HttpStatus move(ResourceLocator* src, ResourceLocator* dstDir, const char* dstName, uint32_t length, bool overwrite) {
			mock("ContentProvider").actualCall("move")
			.withStringParameter("src", src->path.c_str())
			.withStringParameter("dst", (dstDir->path + std::string("/") + std::string(dstName, length)).c_str())
			.withBoolParameter("overwrite", overwrite);
			return HTTP_STATUS_OK;
		}

		// Upload

		static HttpStatus arrangeReceiveInto(ResourceLocator* rl, const char* dstName, uint32_t length) {
			resource = rl;
			mock("ContentProvider").actualCall("receiveInto")
			.withStringParameter("path", (rl->path + std::string("/") + std::string(dstName, length)).c_str());
			temp.clear();
			content.clear();
			workerCalled = false;
			return errAt != ErrAt::OpenForWriting ? HTTP_STATUS_OK : HTTP_STATUS_FORBIDDEN;
		}

		static HttpStatus writeContent(ResourceLocator* rl, const char* buff, uint32_t length) {
			temp += std::string(buff, length);
			CHECK(resource == rl);
			workerCalled = true;
			return errAt != ErrAt::Writing  ? HTTP_STATUS_OK : HTTP_STATUS_FORBIDDEN;
		}

		static HttpStatus contentWritten(ResourceLocator* rl) {
			content = temp;
			CHECK(resource == rl);
			mock("ContentProvider").actualCall("contentWritten");
			return errAt != ErrAt::CloseForWriting ? HTTP_STATUS_OK : HTTP_STATUS_FORBIDDEN;
		}

		// Download

		static HttpStatus arrangeSendFrom(ResourceLocator* rl, uint32_t &size) {
			resource = rl;
			mock("ContentProvider").actualCall("sendFrom")
			.withStringParameter("path", rl->path.c_str());
			workerCalled = false;
			return errAt != ErrAt::OpenForReading ? HTTP_STATUS_OK : HTTP_STATUS_FORBIDDEN;
		}

		static HttpStatus readContent(ResourceLocator* rl) {
			CHECK(resource == rl);
			workerCalled = true;
			return errAt != ErrAt::Reading ? HTTP_STATUS_OK : HTTP_STATUS_FORBIDDEN;
		}

		static HttpStatus contentRead(ResourceLocator* rl) {
			CHECK(resource == rl);
			mock("ContentProvider").actualCall("contentRead");
			return errAt != ErrAt::CloseForReading ? HTTP_STATUS_OK : HTTP_STATUS_FORBIDDEN;
		}

		// Listing

		static HttpStatus arrangeListing(ResourceLocator* rl, bool contents) {
			resource = rl;
			if(contents) {
				mock("ContentProvider").actualCall("listDirectory")
				.withStringParameter("path", rl->path.c_str());
			} else {
				mock("ContentProvider").actualCall("listFile")
				.withStringParameter("path", rl->path.c_str());
			}
			workerCalled = false;
			return errAt != ErrAt::OpenForListing ? HTTP_STATUS_OK : HTTP_STATUS_FORBIDDEN;
		}

		static HttpStatus generateListing(ResourceLocator* rl, const DavProperty* prop) {
			CHECK(resource == rl);
			CHECK(!prop || prop == Types::davProperties);
			workerCalled = true;
			return errAt != ErrAt::Listing ? HTTP_STATUS_OK : HTTP_STATUS_FORBIDDEN;
		}

		bool stepListing(ResourceLocator* rl) {
			return false;
		}

		static HttpStatus listingDone(ResourceLocator* rl) {
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
