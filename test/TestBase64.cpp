/*
 * TestBase64.cpp
 *
 *  Created on: 2017.10.04.
 *      Author: tooma
 */

#include "1test/Test.h"

#include "Base64.h"

#include <string>
#include <string.h>

TEST_GROUP(Base64) {

	struct Put: public Base64::Parser<Put> {
		std::string output;

		inline void byteDecoded(char c) {
			output += c;
		}

		inline void reset() {
			output = "";
			Base64::Parser<Put>::reset();
		}
	} put;

	struct Fut: public Base64::Formater<Fut> {
		std::string output;

		inline void byteEncoded(char c) {
			output += c;
		}

		inline void reset() {
			output = "";
			Base64::Formater<Fut>::reset();
		}
	} fut;

	class Cross: Base64::Formater<Cross>, Base64::Parser<Cross> {
		friend Base64::Formater<Cross>;
		friend Base64::Parser<Cross>;

		inline void byteEncoded(char c) {
			Base64::Parser<Cross>::parse(&c, 1);
		}

		inline void byteDecoded(char c) {
			result += c;
		}

	public:
		inline void reset() {
			result = "";
			Base64::Formater<Cross>::reset();
			Base64::Parser<Cross>::reset();
		}

		inline void done() {
			Base64::Formater<Cross>::done();
			Base64::Parser<Cross>::done();
		}

		using Base64::Formater<Cross>::format;

		std::string result = "";
	} cross;


	void parse(const char* input, const char* expected) {
		for(unsigned int i=0; i<strlen(input)-1; i++) {
			put.reset();
			CHECK(put.parse(input, i));
			CHECK(put.parse(input + i, strlen(input) - i));
			CHECK(put.done());
			CHECK(put.output == expected);
		}
	}

	void format(const char* expected, const char* input) {
		for(unsigned int i=0; i<strlen(input)-1; i++) {
			fut.reset();
			fut.format(input, i);
			fut.format(input + i, strlen(input) - i);
			fut.done();
			CHECK(fut.output == expected);
		}
	}


	static constexpr const char* a64 = "YQ==";
	static constexpr const char* bb64 = "YmI=";
	static constexpr const char* foo64 = "Zm9v";
	static constexpr const char* bar64 = "YmFy";
	static constexpr const char* foobar64 = "Zm9vYmFy";
	static constexpr const char* whatever64 = "d2hhdGV2ZXI=";
	static constexpr const char* dont_care64 = "ZG9uJ3QgY2FyZQ==";
};

TEST(Base64, ParserSanity) {
	parse(foo64, "foo");
	parse(bar64, "bar");
}

TEST(Base64, ParserPadding) {
	parse(foo64, "foo");
	parse(bar64, "bar");
}

TEST(Base64, ParserRestart) {
	parse(foobar64, "foobar");
	parse(whatever64, "whatever");
	parse(dont_care64, "don't care");
}

TEST(Base64, ParserDumbass1) {
	put.reset();
	CHECK(!put.parse("FOO@BAR", 7));
}

TEST(Base64, ParserDumbass2) {
	put.reset();
	CHECK(put.parse("FOO", 3));
	CHECK(!put.done());
}

TEST(Base64, FormatterSanity) {
	format(foo64, "foo");
	format(bar64, "bar");
}

TEST(Base64, FormatterPadding) {
	format(foo64, "foo");
	format(bar64, "bar");
}

TEST(Base64, FormatterRestart) {
	format(foobar64, "foobar");
	format(whatever64, "whatever");
	format(dont_care64, "don't care");
}

TEST(Base64, Back2back) {
	char temp[1024];
	for(int i = 0; i<1024; i++) {
		for(int j=0; j < i; j++)
			temp[j] = i + j;

		cross.reset();
		cross.format(temp, i);
		cross.done();
		CHECK(cross.result == std::string(temp, i));
	}
}
