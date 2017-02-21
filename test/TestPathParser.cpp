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
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "PathParser.h"

#include <stdio.h>
#include <string.h>

namespace {
	static constexpr const char* testString = "/foo/ba r/1 23";
	static constexpr const char* const expected [] = {"foo", "ba r", "1 23"};
}

TEST_GROUP(PathParser) {

    class UUT: public PathParser<UUT> {
            friend PathParser<UUT>;
            char result[sizeof(expected)/sizeof(expected[0])][256];
            unsigned int idx=0, offset=0, nElem = 0;

            void beforeElement()
            {
            	nElem++;
            }

            void parseElement(const char* buff, unsigned int length)
            {
                memcpy(result[idx] + offset, buff, length);
                offset += length;
            }

            void elementDone()
            {
                idx++;
                offset = 0;
            }

        public:
            void check() {
                CHECK(idx == sizeof(expected) / sizeof(expected[0]));
                CHECK(offset == 0);
                CHECK(offset == 0);
                CHECK(nElem == sizeof(expected) / sizeof(expected[0]));
                for(unsigned int i=0; i < sizeof(expected) / sizeof(expected[0]); i++)
                    CHECK(strcmp(expected[i], result[i]) == 0);
            }

            void reset() {
                bzero(result, sizeof(result));
                idx=0;
                offset=0;
                nElem = 0;
                PathParser<UUT>::reset();
            }
    };

    UUT uut;
};

TEST(PathParser, Sanity) {
    uut.reset();
    uut.parsePath(testString, strlen(testString));
    uut.done();
    uut.check();
}

TEST(PathParser, Segmented) {
    for(unsigned int i = 1; i < strlen(testString) - 1; i++) {
    	uut.reset();
        uut.parsePath(testString, i);
        uut.parsePath(testString + i, strlen(testString) - i);
        uut.done();
        uut.check();
    }
}

TEST(PathParser, Segmented3) {
    for(unsigned int i = 1; i < strlen(testString) - 1; i++) {
        for(unsigned int j = i; j < strlen(testString) - 1; j++) {
        	uut.reset();
            uut.parsePath(testString, i);
            uut.parsePath(testString + i, j - i);
            uut.parsePath(testString + j, strlen(testString) - j);
            uut.done();
            uut.check();
        }
    }
}

TEST(PathParser, DirSegmented3) {
	static constexpr const char* testString = "/foo/ba r/1 23/";
	static constexpr const char* const expected [] = {"foo", "ba r", "1 23"};

    for(unsigned int i = 1; i < strlen(testString) - 1; i++) {
        for(unsigned int j = i; j < strlen(testString) - 1; j++) {
        	uut.reset();
            uut.parsePath(testString, i);
            uut.parsePath(testString + i, j - i);
            uut.parsePath(testString + j, strlen(testString) - j);
            uut.done();
            uut.check();
        }
    }
}
