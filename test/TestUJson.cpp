/*
 * TestUJson.cpp
 *
 *      Author: tamas.seller
 */

#include "1test/Test.h"
#include "1test/Mock.h"

#include "UJson.h"

#include <string>
#include <string.h>
#include <iostream> //XXX

namespace {
struct Uut: public UJson<Uut, 32> {
        std::string key, value;
        bool hasType = false;
        ValueType lastType;

        inline void beforeKey() {
            key = "";
        }
        inline void onKey(const char *at, size_t length) {
            key += std::string(at, length);
        }
        inline void afterKey() {
            MOCK(ujson)::CALL(key).withStringParam(key.c_str());}

        inline void beforeValue(ValueType type) {
            CHECK(!hasType);

            if(type == ValueType::String)
                value = "";

            if (type == ValueType::Array) {
                MOCK(ujson)::CALL(enterArray);
                hasType = false;
            } else if (type == ValueType::Object) {
                MOCK(ujson)::CALL(enterObject);
                hasType = false;
            } else {
                lastType = type;
                hasType = true;
            }
        }

        inline void onNull() {MOCK(ujson)::CALL(null);}
        inline void onBoolean(bool x) {MOCK(ujson)::CALL(boolean).withParam(x);}
        inline void onNumber(int32_t value) {MOCK(ujson)::CALL(number).withParam(value);}
        inline void onString(const char *at, size_t length) {
            value += std::string(at, length);

        }
        inline void afterValue(ValueType type) {
            if(type == ValueType::String)
                MOCK(ujson)::CALL(string).withStringParam(value.c_str());
            else if (type == ValueType::Array)
                MOCK(ujson)::CALL(leaveArray);
            else if (type == ValueType::Object)
                MOCK(ujson)::CALL(leaveObject);

            if(hasType)
                CHECK(type == lastType);

            hasType = false;
        }

        inline void onKeyError() {MOCK(ujson)::CALL(keyError);}
        inline void onValueError() {MOCK(ujson)::CALL(valueError);}
        inline void onStructureError() {MOCK(ujson)::CALL(structureError);}
        inline void onResourceError() {MOCK(ujson)::CALL(resourceError);}
    };
}

TEST_GROUP(UJson) {
        Uut uut;

        void expectKey(const char* value) {
            MOCK(ujson)::EXPECT(key).withStringParam(value);
        }

        void expectString(const char* value) {
            MOCK(ujson)::EXPECT(string).withStringParam(value);
        }

        void expectBoolean(bool value) {
            MOCK(ujson)::EXPECT(boolean).withParam(value);
        }

        void expectNumber(int32_t value) {
            MOCK(ujson)::EXPECT(number).withParam(value);
        }

        void expectNull() {
            MOCK(ujson)::EXPECT(null);
        }

        void expectEnterArray() {
            MOCK(ujson)::EXPECT(enterArray);
        }

        void expectEnterObject() {
            MOCK(ujson)::EXPECT(enterObject);
        }

        void expectLeaveArray() {
            MOCK(ujson)::EXPECT(leaveArray);
        }

        void expectLeaveObject() {
            MOCK(ujson)::EXPECT(leaveObject);
        }

        template<class C>
        void process(const char* input, C &&expect) {
            for(unsigned int i=0; i<strlen(input)-1; i++) {
                expect();
                uut.reset();
                uut.parse(input, i);
                uut.parse(input + i, strlen(input) - i);
                uut.done();
            }
        }
    };

TEST(UJson, SimpleObject) {
    process("{\"input\": \"json\", \"foo\" :\"bar\"}", [&](){
        expectEnterObject();
        expectKey("input");
        expectString("json");
        expectKey("foo");
        expectString("bar");
        expectLeaveObject();
    });
}

TEST(UJson, NestedObjects) {
    process(" {\"foo\":{\"bar\":\"baz\"}}", [&](){
        expectEnterObject();
        expectKey("foo");
        expectEnterObject();
        expectKey("bar");
        expectString("baz");
        expectLeaveObject();
        expectLeaveObject();
    });
}

TEST(UJson, NestedArrays) {
    process("[[\"foo\", \"bar\"], \"baz\"]", [&](){
        expectEnterArray();
        expectEnterArray();
        expectString("foo");
        expectString("bar");
        expectLeaveArray();
        expectString("baz");
        expectLeaveArray();
    });
}

TEST(UJson, MixedNested) {
    process("[{\"foo\":\"bar\", \"foobar\":\"baz\"}"
            "\"whatever\"]", [&](){
        expectEnterArray();
        expectEnterObject();
        expectKey("foo");
        expectString("bar");
        expectKey("foobar");
        expectString("baz");
        expectLeaveObject();
        expectString("whatever");
        expectLeaveArray();
    });
}
