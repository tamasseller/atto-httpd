/*
 * TestJsonParser.cpp
 *
 *  Created on: 2017.10.04.
 *      Author: tooma
 */

#ifndef TESTJSONPARSER_CPP_
#define TESTJSONPARSER_CPP_

#include "1test/Test.h"
#include "1test/Mock.h"

#include "JsonParser.h"

TEST_GROUP(JsonParser)
{
	static constexpr const char* rootNumberTestDocument = "42";
	static constexpr const char* rootStringTestDocument = "\"foo\"";
	static constexpr const char* rootTrueTestDocument = "true";
	static constexpr const char* arrayTestDocument = "[\"0\", \"1\", \"2\", \"3\", \"4\", null, true, false]";
	static constexpr const char* nestedArrayTestDocument = "[[1, 2, 3], [4, 5, 6], [7, 8, 9]]";
	static constexpr const char* telescopicArrayTestDocument = "[0, [1, [2, [3, [4, [5]]]]]]";
	static constexpr const char* objectTestDocument = "{\"one\":1, \"two\" : 2, \"three\": 3, \"four\" :4}";
	static constexpr const char* nestObjectTestDocument = "{\"a\":{\"a\":0,\"b\":1},\"b\":{\"a\":2,\"b\":3}}";
	static constexpr const char* erroneousTestDocument = "{0:\"meaning\", \"stupid\": [\"shit\"}, \"and\": \"one correct\", \"more\": bullshit, \"deep\":[[[[[[[[]]]]]]]]";
	static constexpr const char* complexTestDocument =
		"{\r\n"
		"  \"id\": \"0001\",\r\n"
		"  \"type\": \"donut\",\r\n"
		"  \"name\": \"Cake\",\r\n"
		"  \"ppu\": 0.55,\r\n"
		"  \"batters\":\r\n"
		"    {\r\n"
		"      \"batter\":\r\n"
		"        [\r\n"
		"          { \"id\": 1001, \"type\": \"Regular\" },\r\n"
		"          { \"id\": 1002, \"type\": \"Chocolate\" },\r\n"
		"          { \"id\": 1003, \"type\": \"Blueberry\" },\r\n"
		"          { \"id\": 1004, \"type\": \"Devil's Food\" }\r\n"
		"        ]\r\n"
		"    },\r\n"
		"  \"topping\":\r\n"
		"    [\r\n"
		"      { \"id\": 5001, \"type\": \"None\" },\r\n"
		"      { \"id\": 5002, \"type\": \"Glazed\" },\r\n"
		"      { \"id\": 5005, \"type\": \"Sugar\" },\r\n"
		"      { \"id\": 5007, \"type\": \"Powdered Sugar\" },\r\n"
		"      { \"id\": 5006, \"type\": \"Chocolate with Sprinkles\" },\r\n"
		"      { \"id\": 5003, \"type\": \"Chocolate\" },\r\n"
		"      { \"id\": 5004, \"type\": \"Maple\" }\r\n"
		"    ]\r\n"
		"}\r\n";

	JsonParser<8> uut;
};

TEST(JsonParser, RootNumber) {
	int x = 0;
	NumberExtractor xExtractor(x);

	uut.reset(&xExtractor);
	CHECK(uut.parse(rootNumberTestDocument, strlen(rootNumberTestDocument)));
	CHECK(uut.done());

	CHECK(x == 42);
}

TEST(JsonParser, RootString) {
	char x[8];
	auto xExtractor = makeStringExtractor(x);

	uut.reset(&xExtractor);
	CHECK(uut.parse(rootStringTestDocument, strlen(rootStringTestDocument)));
	CHECK(uut.done());

	CHECK(pet::Str::cmp(x, "foo"));
}

TEST(JsonParser, RootTrue) {
	bool x = false;
	BoolExtractor xExtractor(x);

	uut.reset(&xExtractor);
	CHECK(uut.parse(rootTrueTestDocument, strlen(rootTrueTestDocument)));
	CHECK(uut.done());

	CHECK(x);
}

TEST(JsonParser, SimpleArray) {
	char x[8], y[8];

	auto xExtractor = makeStringExtractor(x);
	auto yExtractor = makeStringExtractor(y);

	auto filter = assemble<ArrayFilter>(
		FilterEntry(1u, &xExtractor),
		FilterEntry(3u, &yExtractor)
	);

	uut.reset(&filter);
	CHECK(uut.parse(arrayTestDocument, strlen(arrayTestDocument)));
	CHECK(uut.done());

	CHECK(pet::Str::cmp(x, "1"));
	CHECK(pet::Str::cmp(y, "3"));
}

TEST(JsonParser, NestedArray) {
	int x[4];
	NumberExtractor extractors[] = {x[0], x[1], x[2], x[3]};

	auto firstRow = assemble<ArrayFilter>(FilterEntry(1u, extractors + 0));
	auto secondRow = assemble<ArrayFilter>(FilterEntry(0u, extractors + 1), FilterEntry(2u, extractors + 2));
	auto thirdRow = assemble<ArrayFilter>(FilterEntry(1u, extractors + 3));
	auto filter = assemble<ArrayFilter>(
			FilterEntry(0u, &firstRow),
			FilterEntry(1u, &secondRow),
			FilterEntry(2u, &thirdRow)
	);

	uut.reset(&filter);
	CHECK(uut.parse(nestedArrayTestDocument, strlen(nestedArrayTestDocument)));
	CHECK(uut.done());

	CHECK(x[0] == 2);
	CHECK(x[1] == 4);
	CHECK(x[2] == 6);
	CHECK(x[3] == 8);
}

TEST(JsonParser, TelescopicArray) {
	int x[6];
	NumberExtractor extractors[] = {x[0], x[1], x[2], x[3], x[4], x[5]};

	auto f5 = assemble<ArrayFilter>(FilterEntry(0u, extractors + 5));
	auto f4 = assemble<ArrayFilter>(FilterEntry(1u, &f5), FilterEntry(0u, extractors + 4));
	auto f3 = assemble<ArrayFilter>(FilterEntry(1u, &f4), FilterEntry(0u, extractors + 3));
	auto f2 = assemble<ArrayFilter>(FilterEntry(1u, &f3), FilterEntry(0u, extractors + 2));
	auto f1 = assemble<ArrayFilter>(FilterEntry(1u, &f2), FilterEntry(0u, extractors + 1));
	auto filter = assemble<ArrayFilter>(FilterEntry(1u, &f1), FilterEntry(0u, extractors + 0));

	uut.reset(&filter);
	CHECK(uut.parse(telescopicArrayTestDocument, strlen(telescopicArrayTestDocument)));
	CHECK(uut.done());

	for(unsigned int i = 0; i<sizeof(x)/sizeof(x[0]); i++)
		CHECK(x[i] == (int)i);
}

TEST(JsonParser, SimpleObject) {
	int x[4];
	NumberExtractor extractors[] = {x[0], x[1], x[2], x[3]};

	auto filter = assemble<ObjectFilter>(
			FilterEntry("three", extractors + 2),
			FilterEntry("two", extractors + 1),
			FilterEntry("four", extractors + 3),
			FilterEntry("one", extractors + 0)
		);

	uut.reset(&filter);
	CHECK(uut.parse(objectTestDocument, strlen(objectTestDocument)));
	CHECK(uut.done());

	for(unsigned int i = 0; i<sizeof(x)/sizeof(x[0]); i++)
		CHECK(x[i] == (int)i+1);
}

TEST(JsonParser, NestedObjects) {
	int x[2];
	NumberExtractor extractors[] = {x[0], x[1]};

	auto f1 = assemble<ObjectFilter>(FilterEntry("b", extractors + 0));
	auto f2 = assemble<ObjectFilter>(FilterEntry("a", extractors + 1));

	auto filter = assemble<ObjectFilter>(
			FilterEntry("a", &f1),
			FilterEntry("b", &f2)
		);

	uut.reset(&filter);
	CHECK(uut.parse(nestObjectTestDocument, strlen(nestObjectTestDocument)));
	CHECK(uut.done());

	CHECK(x[0] == 1);
	CHECK(x[1] == 2);
}

TEST(JsonParser, Complex) {
	char type[16] = {0,};
	int id = 0;

	auto typeExtractor = makeStringExtractor(type);
	NumberExtractor glazedIdExtractor(id);
	auto f2 = assemble<ObjectFilter>(FilterEntry("id", &glazedIdExtractor));
	auto f1 = assemble<ArrayFilter>(FilterEntry(1, &f2));
	auto filter = assemble<ObjectFilter>(
			FilterEntry("type", &typeExtractor),
			FilterEntry("topping", &f1)
		);

	uut.reset(&filter);
	CHECK(uut.parse(complexTestDocument, strlen(complexTestDocument)));
	CHECK(uut.done());

	CHECK(pet::Str::cmp(type, "donut"));
	CHECK(id == 5002);
}

TEST(JsonParser, Erroneous) {
	char never[16] = {0,};
	auto typeExtractor = makeStringExtractor(never);
	auto filter = assemble<ObjectFilter>(FilterEntry("and", &typeExtractor));

	uut.reset(&filter);
	CHECK(uut.parse(erroneousTestDocument, strlen(erroneousTestDocument)));
	CHECK(uut.done());

	CHECK(pet::Str::cmp(never, "") == 0);
}


#endif /* TESTJSONPARSER_CPP_ */
