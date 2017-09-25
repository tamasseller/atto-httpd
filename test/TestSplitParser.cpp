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
