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

#include "KvParser.h"

#include <stdio.h>
#include <string.h>

TEST_GROUP(KvParser) {
    class UUT: public KvParser<UUT> {
            friend KvParser<UUT>;
            char resultKey[256], resultValue[256];
            unsigned int keyOffset, valueOffset;
            bool isKeyDone, isValueDone;

            void parseKey(const char * buff, unsigned int len) {
                memcpy(resultKey + keyOffset, buff, len);
                keyOffset += len;
            }

            void parseValue(const char * buff, unsigned int len) {
                memcpy(resultValue + valueOffset, buff, len);
                valueOffset += len;
            }

            void keyDone() {
                isKeyDone = true;
            }

            void valueDone() {
                isValueDone = true;
            }
    public:
            void reset() {
            	KvParser<UUT>::reset();
            	bzero(resultKey, sizeof(resultKey));
            	bzero(resultValue, sizeof(resultValue));
                keyOffset=0;
                valueOffset=0;
                isKeyDone = false;
                isValueDone = false;
            }

            void check(const char* expectedKey, const char* expectedValue) {

                CHECK(keyOffset == strlen(expectedKey));
                CHECK(strcmp(expectedKey, resultKey) == 0);

                if(expectedValue) {
                	CHECK(valueOffset == strlen(expectedValue));
                	CHECK(strcmp(expectedValue, resultValue) == 0);
                	CHECK(isValueDone);
                } else {
                	CHECK(!valueOffset);
                	CHECK(!isValueDone);
                }

                CHECK(isKeyDone);
            }

            static void run(const char* testString, const char* expectedKey, const char* expectedValue)
            {
            	union {
                	UUT uut;
            	};

            	uut.reset();
				uut.progressWithKv(testString, strlen(testString));
				uut.kvDone();
				uut.check(expectedKey, expectedValue);

				for(unsigned int i = 1; i < strlen(testString) - 1; i++) {
					uut.reset();
					uut.progressWithKv(testString, i);
					uut.progressWithKv(testString + i, strlen(testString) - i);
					uut.kvDone();
					uut.check(expectedKey, expectedValue);
                }

                for(unsigned int i = 1; i < strlen(testString) - 1; i++) {
                    for(unsigned int j = i; j < strlen(testString) - 1; j++) {
                    	uut.reset();
                        uut.progressWithKv(testString, i);
                        uut.progressWithKv(testString + i, j - i);
                        uut.progressWithKv(testString + j, strlen(testString) - j);
                        uut.kvDone();
                        uut.check(expectedKey, expectedValue);
                    }
                }
            }
    };
};

TEST(KvParser, Sanity) {
    UUT::run("foo= \"bar\"", "foo", "bar");
}

TEST(KvParser, SpaceValue) {
    UUT::run("foo=\"bar baz\"", "foo", "bar baz");
}

TEST(KvParser, SpaceOutside) {
    UUT::run("foo= \"bar baz\" ", "foo", "bar baz");
}

TEST(KvParser, Equal) {
    UUT::run("foo= \"bar==baz\" ", "foo", "bar==baz");
}

TEST(KvParser, NoValue) {
    UUT::run("foo ", "foo", nullptr);
}

TEST(KvParser, Reset) {
    UUT uut;
    uut.reset();
    const char* testString = "abc=\"123\"";
    const char* expectedKey = "abc";
    const char* expectedValue = "123";

    uut.progressWithKv(testString, strlen(testString));
    uut.kvDone();
    uut.check(expectedKey, expectedValue);
    uut.reset();
    uut.progressWithKv(testString, strlen(testString));
    uut.kvDone();
    uut.check(expectedKey, expectedValue);
}
