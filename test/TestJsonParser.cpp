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
	static constexpr const char* testDocument =
		"{\r\n"
		"  \"id\": \"0001\",\r\n"
		"  \"type\": \"donut\",\r\n"
		"  \"name\": \"Cake\",\r\n"
		"  \"ppu\": 0.55,\r\n"
		"  \"batters\":\r\n"
		"    {\r\n"
		"      \"batter\":\r\n"
		"        [\r\n"
		"          { \"id\": \"1001\", \"type\": \"Regular\" },\r\n"
		"          { \"id\": \"1002\", \"type\": \"Chocolate\" },\r\n"
		"          { \"id\": \"1003\", \"type\": \"Blueberry\" },\r\n"
		"          { \"id\": \"1004\", \"type\": \"Devil's Food\" }\r\n"
		"        ]\r\n"
		"    },\r\n"
		"  \"topping\":\r\n"
		"    [\r\n"
		"      { \"id\": \"5001\", \"type\": \"None\" },\r\n"
		"      { \"id\": \"5002\", \"type\": \"Glazed\" },\r\n"
		"      { \"id\": \"5005\", \"type\": \"Sugar\" },\r\n"
		"      { \"id\": \"5007\", \"type\": \"Powdered Sugar\" },\r\n"
		"      { \"id\": \"5006\", \"type\": \"Chocolate with Sprinkles\" },\r\n"
		"      { \"id\": \"5003\", \"type\": \"Chocolate\" },\r\n"
		"      { \"id\": \"5004\", \"type\": \"Maple\" }\r\n"
		"    ]\r\n"
		"}\r\n";

	JsonParser<32> uut;
};

TEST(JsonParser, Sanity) {

	uut.reset();
}


#endif /* TESTJSONPARSER_CPP_ */
