/*
 * TestKeywords.cpp
 *
 *  Created on: 2017.01.21.
 *      Author: tooma
 */

#include "CppUTest/TestHarness.h"

#include "Keywords.h"

const int beforeKeywords = -1;

typedef Keywords<int, 7> TestKeywords;

const TestKeywords keywords = {
	TestKeywords::Keyword("foobar", 1),
	TestKeywords::Keyword("some", 2),
	TestKeywords::Keyword("foo", 3),
	TestKeywords::Keyword("things", 4),
	TestKeywords::Keyword("bar", 5),
	TestKeywords::Keyword("buz", 6),
	TestKeywords::Keyword("baz", 7)
};

typedef Keywords<int, 2> VoidKeywords;

const int afterKeywords = 1;

TEST_GROUP(Keywords) {
	TestKeywords::Matcher matcher;
};

TEST(Keywords, Constant) {
	const int *a = &beforeKeywords, *c = &afterKeywords;
	const int *b = (int*)&keywords;

	CHECK((a < b && b < c) || (a > b && b > c)); // XXX
}

TEST(Keywords, Segmented) {
	matcher.reset();
	CHECK(matcher.progress(keywords, "f"));
	CHECK(matcher.progress(keywords, "oob"));
	CHECK(matcher.progress(keywords, "ar"));
	CHECK(matcher.match(keywords)->getValue() == 1);
}

TEST(Keywords, Longer) {
	matcher.reset();
	CHECK(!matcher.progress(keywords, "foobarr"));
	CHECK(!matcher.match(keywords));
}

TEST(Keywords, Nonexistent) {
	matcher.reset();
	CHECK(!matcher.progress(keywords, "asd"));
	CHECK(!matcher.progress(keywords, "qwe"));
	CHECK(matcher.match(keywords) == 0);
}

TEST(Keywords, PartialMatch) {
	matcher.reset();
	CHECK(matcher.progress(keywords, "fo"));
	CHECK(matcher.progress(keywords, "o"));
	CHECK(matcher.match(keywords)->getValue() == 3);
}

TEST(Keywords, PartialMatchMove) {
	matcher.reset();
	CHECK(matcher.progress(keywords, "baz"));
	CHECK(matcher.match(keywords)->getValue() == 7);
}

TEST(Keywords, PartialNoMatch) {
	matcher.reset();
	CHECK(matcher.progress(keywords, "so"));
	CHECK(matcher.match(keywords) == 0);
}
