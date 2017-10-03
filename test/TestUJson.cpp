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
    process("{\"input\": 3141, \"foo\" :\"bar\"}", [&](){
        expectEnterObject();
        expectKey("input");
        expectNumber(3141);
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

TEST(UJson, LiteralsInObject) {
    process("{\"foo\": false, \"bar\": true, \"baz\": null}", [&](){
        expectEnterObject();
        expectKey("foo");
        expectBoolean(false);
        expectKey("bar");
        expectBoolean(true);
        expectKey("baz");
        expectNull();
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

TEST(UJson, MatricArrays) {
    process("[[1, -2, 3], [-4, 5, -6], [7, -8, 9]]", [&](){
        expectEnterArray();
        expectEnterArray();
        expectNumber(1);
        expectNumber(-2);
        expectNumber(3);
        expectLeaveArray();
        expectEnterArray();
        expectNumber(-4);
        expectNumber(5);
        expectNumber(-6);
        expectLeaveArray();
        expectEnterArray();
        expectNumber(7);
        expectNumber(-8);
        expectNumber(9);
        expectLeaveArray();
        expectLeaveArray();
    });
}

TEST(UJson, MixedNested) {
    process("[{\"foo\":\"bar\", \"foobar\":\"baz\"},"
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

TEST(UJson, BunchOfStuffz) {
    process("{"
            "  \"hero\": {"
            "    \"xpos\": 844,"
            "    \"ypos\": 511,"
            "    \"id\": 59,"
            "    \"name\": \"npc_dota_hero_huskar\","
            "    \"level\": 25,"
            "    \"alive\": true,"
            "    \"respawn_seconds\": 0,"
            "    \"buyback_cost\": 1231,"
            "    \"buyback_cooldown\": 0,"
            "    \"health\": 2682,"
            "    \"max_health\": 2875,"
            "    \"health_percent\": 93,"
            "    \"mana\": 944,"
            "    \"max_mana\": 944,"
            "    \"mana_percent\": 100,"
            "    \"silenced\": false,"
            "    \"stunned\": false,"
            "    \"disarmed\": false,"
            "    \"magicimmune\": false,"
            "    \"hexed\": false,"
            "    \"muted\": false,"
            "    \"break\": false,"
            "    \"has_debuff\": false,"
            "    \"talent_1\": true,"
            "    \"talent_2\": false,"
            "    \"talent_3\": false,"
            "    \"talent_4\": true,"
            "    \"talent_5\": true,"
            "    \"talent_6\": false,"
            "    \"talent_7\": true,"
            "    \"talent_8\": false"
            "  },"
            "  \"previously\": {"
            "    \"hero\": {"
            "      \"health\": 2791,"
            "      \"health_percent\": 97"
            "    }"
            "  }"
            "}", [&](){
        expectEnterObject();
        expectKey("hero");
        expectEnterObject();
        expectKey("xpos");
        expectNumber(844);
        expectKey("ypos");
        expectNumber(511);
        expectKey("id");
        expectNumber(59);
        expectKey("name");
        expectString("npc_dota_hero_huskar");
        expectKey("level");
        expectNumber(25);
        expectKey("alive");
        expectBoolean(true);
        expectKey("respawn_seconds");
        expectNumber(0);
        expectKey("buyback_cost");
        expectNumber(1231);
        expectKey("buyback_cooldown");
        expectNumber(0);
        expectKey("health");
        expectNumber(2682);
        expectKey("max_health");
        expectNumber(2875);
        expectKey("health_percent");
        expectNumber(93);
        expectKey("mana");
        expectNumber(944);
        expectKey("max_mana");
        expectNumber(944);
        expectKey("mana_percent");
        expectNumber(100);
        expectKey("silenced");
        expectBoolean(false);
        expectKey("stunned");
        expectBoolean(false);
        expectKey("disarmed");
        expectBoolean(false);
        expectKey("magicimmune");
        expectBoolean(false);
        expectKey("hexed");
        expectBoolean(false);
        expectKey("muted");
        expectBoolean(false);
        expectKey("break");
        expectBoolean(false);
        expectKey("has_debuff");
        expectBoolean(false);
        expectKey("talent_1");
        expectBoolean(true);
        expectKey("talent_2");
        expectBoolean(false);
        expectKey("talent_3");
        expectBoolean(false);
        expectKey("talent_4");
        expectBoolean(true);
        expectKey("talent_5");
        expectBoolean(true);
        expectKey("talent_6");
        expectBoolean(false);
        expectKey("talent_7");
        expectBoolean(true);
        expectKey("talent_8");
        expectBoolean(false);
        expectLeaveObject();
        expectKey("previously");
        expectEnterObject();
        expectKey("hero");
        expectEnterObject();
        expectKey("health");
        expectNumber(2791);
        expectKey("health_percent");
        expectNumber(97);
        expectLeaveObject();
        expectLeaveObject();
        expectLeaveObject();
    });
}


