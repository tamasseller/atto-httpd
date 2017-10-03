/*
 * TestIntParser.cpp
 *
 *      Author: tamas.seller
 */

#include "1test/Test.h"

#include "IntParser.h"
#include <string.h>

TEST_GROUP(IntParser) {
    IntParser uut;

    void process(const char* str, int expected) {
        for(unsigned int i=0; i<strlen(str)-1; i++) {
            uut.reset();
            CHECK(uut.parseInt(str, i));
            CHECK(uut.parseInt(str + i, strlen(str) - i));
            CHECK(uut.getData() == expected);
        }
    }

    bool processBad(const char* str) {
        bool ret = true;
        for(unsigned int i=0; i<strlen(str)-1; i++) {
            uut.reset();
            ret = uut.parseInt(str, i);
            ret = uut.parseInt(str + i, strlen(str) - i) && ret;
        }

        return ret;
    }
};

TEST(IntParser, One) {
    process("1", 1);
}

TEST(IntParser, Sanity) {
    process("42", 42);
}

TEST(IntParser, Negative) {
    process("-42", -42);
}

TEST(IntParser, Max)
{
    process("2147483647", 2147483647);
}

TEST(IntParser, Min)
{
    process("2147483648", -2147483648);
}

// TODO Don't care to implement over/underflow checking right now.
IGNORE_TEST(IntParser, OverMax)
{
    CHECK(!processBad("9999999999"));
}
