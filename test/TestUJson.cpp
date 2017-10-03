/*
 * TestUJson.cpp
 *
 *      Author: tamas.seller
 */

#include "BaseUJsonTest.h"

TEST(UJson, RootString) {
	process("\"foo\"", [&](){
		expectString("foo");
	});
}

TEST(UJson, RootNull) {
	process("null", [&](){
		expectNull();
	});
}

TEST(UJson, RootTrue) {
	process("true", [&](){
		expectBoolean(true);
	});
}

TEST(UJson, RootFalse) {
	process("false", [&](){
		expectBoolean(false);
	});
}

TEST(UJson, RootNumber) {
	process("123", [&](){
		expectNumber(123);
	});
}

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

TEST(UJson, MatrixArrays) {
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

TEST(UJson, Quoting) {
    process("{\"new\\r\\n\\tline\": \"\\/\\\\\\f\\b\"}", [&](){
        expectEnterObject();
        expectKey("new\r\n\tline");
        expectString("/\\\f\b");
        expectLeaveObject();
    });
}

TEST(UJson, EmptyString) {
    process("\"\"", [&](){
        expectString("");
    });
}

TEST(UJson, EmptyArray) {
    process("[]", [&](){
        expectEnterArray();
        expectLeaveArray();
    });
}

TEST(UJson, EmptyObject) {
    process("{}", [&](){
        expectEnterObject();
        expectLeaveObject();
    });
}

TEST(UJson, MultipleEmpty) {
    process("{\"\": [{}, []]}", [&](){
        expectEnterObject();
        expectKey("");
        expectEnterArray();
        expectEnterObject();
        expectLeaveObject();
        expectEnterArray();
        expectLeaveArray();
        expectLeaveArray();
        expectLeaveObject();
    });
}

TEST(UJson, BunchOfStuffz) {
    process("{\r\n"
            "\t\"hero\": {\r\n"
            "\t\t\"xpos\": 844,\r\n"
            "\t\t\"ypos\": 511,\r\n"
            "\t\t\"id\": 59,\r\n"
            "\t\t\"name\": \"npc_dota_hero_huskar\",\r\n"
            "\t\t\"level\": 25,\r\n"
            "\t\t\"alive\": true,\r\n"
            "\t\t\"respawn_seconds\": 0,\r\n"
            "\t\t\"buyback_cost\": 1231,\r\n"
            "\t\t\"buyback_cooldown\": 0,\r\n"
            "\t\t\"health\": 2682,\r\n"
            "\t\t\"max_health\": 2875,\r\n"
            "\t\t\"health_percent\": 93,\r\n"
            "\t\t\"mana\": 944,\r\n"
            "\t\t\"max_mana\": 944,\r\n"
            "\t\t\"mana_percent\": 100,\r\n"
            "\t\t\"silenced\": false,\r\n"
            "\t\t\"stunned\": false,\r\n"
            "\t\t\"disarmed\": false,\r\n"
            "\t\t\"magicimmune\": false,\r\n"
            "\t\t\"hexed\": false,\r\n"
            "\t\t\"muted\": false,\r\n"
            "\t\t\"break\": false,\r\n"
            "\t\t\"has_debuff\": false,\r\n"
            "\t\t\"talent_1\": true,\r\n"
            "\t\t\"talent_2\": false,\r\n"
            "\t\t\"talent_3\": false,\r\n"
            "\t\t\"talent_4\": true,\r\n"
            "\t\t\"talent_5\": true,\r\n"
            "\t\t\"talent_6\": false,\r\n"
            "\t\t\"talent_7\": true,\r\n"
            "\t\t\"talent_8\": false\r\n"
            "\t},\r\n"
            "\t\"previously\": {\r\n"
            "\t\t\"hero\": {\r\n"
            "\t\t\t\"health\": 2791,\r\n"
            "\t\t\t\"health_percent\": 97\r\n"
    		"\t\t}\r\n"
            "\t}\r\n"
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
