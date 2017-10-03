/*
 * TestUJsonAbuse.cpp
 *
 *  Created on: 2017.10.04.
 *      Author: tooma
 */

#include "BaseUJsonTest.h"

TEST(UJson, ValueQuotingError) {
    process("\"\\a\"", [&](){
        expectValueError();
        expectString("");
    });
}

TEST(UJson, KeyQuotingError) {
    process("{\"\\a\":\"\\b\"}", [&](){
    	expectEnterObject();
        expectKeyError();
        expectKey("");
        expectString("\b");
        expectLeaveObject();
    });
}

TEST(UJson, Dumbass1) {
    process("}", [&](){
        expectStructureError();
    });
}

TEST(UJson, Dumbass2) {
    process("]", [&](){
        expectStructureError();
    });
}

TEST(UJson, Dumbass3) {
    process("\"", [&](){
        expectValueError();
    });
}

TEST(UJson, Dumbass4) {
    process("1, 2", [&](){
    	expectNumber(1);
        expectStructureError();
        expectNumber(2);
    });
}

TEST(UJson, Dumbass5) {
    process("{foo\"\":bar\"\"}", [&](){
        expectEnterObject();
        expectKeyError();
        expectKeyError();
        expectKeyError();
        expectKey("");
        expectValueError();
        expectLeaveObject();
    });
}

TEST(UJson, Dumbass6) {
    process("tru", [&](){
    	expectValueError();
    });
}

TEST(UJson, Dumbass7) {
    process("[0}", [&](){
        expectEnterArray();
        expectNumber(0);
    	expectStructureError();
        expectLeaveArray();
    });
}

TEST(UJson, Dumbass8) {
    process("{\"a\": 0]", [&](){
        expectEnterObject();
        expectKey("a");
        expectNumber(0);
    	expectStructureError();
        expectLeaveObject();
    });
}

TEST(UJson, Dumbass9) {
    process("0]", [&](){
        expectNumber(0);
    	expectStructureError();
    });
}

TEST(UJson, Dumbass10) {
    process("{\"x\" -> 0}", [&](){
        expectEnterObject();
        expectKey("x");
    	expectKeyError();
    	expectKeyError();
    	expectKeyError();
    	expectLeaveObject();
    });
}

TEST(UJson, Dumbass11) {
    process("\"x", [&](){
    	expectString("x");
        expectStructureError();
    });
}

TEST(UJson, Dumbass12) {
    process("\"x\\", [&](){
    	expectString("x");
        expectStructureError();
    });
}

TEST(UJson, Overload) {
    process("[[[[[[[[ [[[[[[[[ [[[[[[[[ [[[[[[[[ [{", [&](){
    	for(int i=0; i<32; i++)
    		expectEnterArray();

        expectResourceError();
        expectResourceError();
        expectStructureError();
    });
}
