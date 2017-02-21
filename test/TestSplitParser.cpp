/*
 * TestSplitParser.cpp
 *
 *  Created on: 2017.02.06.
 *      Author: tooma
 */

#include "CppUTest/TestHarness.h"

#include "SplitParser.h"

#include <stdio.h>
#include <string.h>

namespace {
	static constexpr const char* testString = "Digest username=\"admin from hell\", realm=\"webdav\", nonce=\"gxnpSNxHBQA=53bf38f0b5840421c034dafc9110bff733093561\", uri=\"/webdav/\", response=\"6c0fd83665a92b0db57d92978611eb5c\", algorithm=\"MD5\", cnonce=\"3ab39933c310098c800e8789a1c1616e\", nc=00000006, qop=\"auth\"";
	static constexpr const char* const expected [] = {"Digest",
							 "username=\"admin from hell\"",
							 "realm=\"webdav\"",
							 "nonce=\"gxnpSNxHBQA=53bf38f0b5840421c034dafc9110bff733093561\"",
							 "uri=\"/webdav/\"",
							 "response=\"6c0fd83665a92b0db57d92978611eb5c\"",
							 "algorithm=\"MD5\"",
							 "cnonce=\"3ab39933c310098c800e8789a1c1616e\"",
							 "nc=00000006",
							 "qop=\"auth\""};
}


TEST_GROUP(SplitParser) {

    class UUT: public Splitter<UUT> {
            friend Splitter<UUT>;
            char result[sizeof(expected)/sizeof(expected[0])][256];
            unsigned int idx, offset;

            void parseField(const char* buff, unsigned int length)
            {
                memcpy(result[idx] + offset, buff, length);
                offset += length;
            }
            void fieldDone()
            {
                idx++;
                offset = 0;
            }
        public:
            void check() {
                CHECK(idx == sizeof(expected) / sizeof(expected[0]));
                CHECK(offset == 0);
                for(unsigned int i=0; i < sizeof(expected) / sizeof(expected[0]); i++)
                    CHECK(strcmp(expected[i], result[i]) == 0);
            }

            void reset() {
                bzero(result, sizeof(result));
                idx=0;
                offset=0;
            	Splitter<UUT>::reset();
            }
    };

    union {
    	UUT uut;
    };
};

TEST(SplitParser, Sanity) {
	uut.reset();
    uut.progressWithSplitting(testString, strlen(testString));
    uut.splittingDone();
    uut.check();
}

TEST(SplitParser, Segmented) {
    for(unsigned int i = 1; i < strlen(testString) - 1; i++) {
    	uut.reset();
        uut.progressWithSplitting(testString, i);
        uut.progressWithSplitting(testString + i, strlen(testString) - i);
        uut.splittingDone();
        uut.check();
    }
}

TEST(SplitParser, Segmented3) {
    for(unsigned int i = 1; i < strlen(testString) - 1; i++) {
        for(unsigned int j = i; j < strlen(testString) - 1; j++) {
        	uut.reset();
            uut.progressWithSplitting(testString, i);
            uut.progressWithSplitting(testString + i, j - i);
            uut.progressWithSplitting(testString + j, strlen(testString) - j);
            uut.splittingDone();
            uut.check();
        }
    }
}
