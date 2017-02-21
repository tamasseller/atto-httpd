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
#include "CppUTest/TestHarness.h"

#include "AuthDigest.h"

TEST_GROUP(AuthDigest) {
	struct AuthProvider {
		static constexpr const char* username = "test";
		static constexpr const char* realm = "test";
		static constexpr const char* RFC2069_A1 = "aeeebbfd75d1499d24388f5b9b10e0ef";
	};

	typedef AuthDigest<AuthProvider> Uut;

	union {
		Uut uut;
	};
};

TEST(AuthDigest, Happy) {
	const char *testString = "Digest "
			"username=\"test\", "
			"realm=\"test\", "
			"nonce=\"verysecurenonce\", "
			"uri=\"/\", "
			"response=\"d20d272dd6ec2d9f135a6c109e71415b\", ";

	uut.reset("GET");
	uut.parseAuthField(testString, strlen(testString));
	uut.authFieldDone();

	CHECK(uut.isAuthorized());
}

TEST(AuthDigest, Segmented) {
	const char *testString = "Digest "
			"username=\"test\", "
			"realm=\"test\", "
			"nonce=\"verysecurenonce\", "
			"uri=\"/\", "
			"response=\"d20d272dd6ec2d9f135a6c109e71415b\", "
			"algorithm=\"MD5\", ";

	for(unsigned int i=0; i<strlen(testString); i++) {
		uut.reset("GET");
		uut.parseAuthField(testString, i);
		uut.parseAuthField(testString + i, strlen(testString) - i);
		uut.authFieldDone();

		CHECK(uut.isAuthorized());
	}
}

TEST(AuthDigest, RepeatedOk) {
	const char *testString = "Digest "
			"username=\"test\", "
			"realm=\"test\", "
			"nonce=\"verysecurenonce\", "
			"uri=\"/\", "
			"response=\"d20d272dd6ec2d9f135a6c109e71415b\", ";

	uut.reset("GET");
	uut.parseAuthField(testString, strlen(testString));
	uut.authFieldDone();

	CHECK(uut.isAuthorized());

	uut.reset("GET");
	uut.parseAuthField(testString, strlen(testString));
	uut.authFieldDone();

	CHECK(uut.isAuthorized());
}

TEST(AuthDigest, NokThenOk) {
	const char *testString = "Digest "
			"username=\"test\", "
			"realm=\"test\", "
			"nonce=\"verysecurenonce\", "
			"uri=\"/\", "
			"response=\"d20d272dd6ec2d9f135a6c109e71415b\", ";

	uut.reset("PUT");
	uut.parseAuthField(testString, strlen(testString));
	uut.authFieldDone();

	CHECK(!uut.isAuthorized());

	uut.reset("GET");
	uut.parseAuthField(testString, strlen(testString));
	uut.authFieldDone();

	CHECK(uut.isAuthorized());
}

TEST(AuthDigest, IllFormedThenOk) {
	const char *testString = "Digest "
			"username=\"test\", "
			"realm=\"test\", "
			"nonce=\"verysecurenonce\", "
			"uri=\"/\", "
			"response=\"d20d272dd6ec2d9f135a6c109e71415b\", ";

	const char *testIll = "Digest "
			"username=\"asd";

	uut.reset("GET");
	uut.parseAuthField(testIll, strlen(testIll));
	uut.authFieldDone();

	CHECK(!uut.isAuthorized());

	uut.reset("GET");
	uut.parseAuthField(testString, strlen(testString));
	uut.authFieldDone();

	CHECK(uut.isAuthorized());
}

TEST(AuthDigest, OkThenIllFormed) {
	const char *testString = "Digest "
			"username=\"test\", "
			"realm=\"test\", "
			"nonce=\"verysecurenonce\", "
			"uri=\"/\", "
			"response=\"d20d272dd6ec2d9f135a6c109e71415b\", ";

	const char *testIll = "Digest "
			"username=\"asd";

	uut.reset("GET");
	uut.parseAuthField(testString, strlen(testString));
	uut.authFieldDone();

	CHECK(uut.isAuthorized());

	uut.reset("GET");
	uut.parseAuthField(testIll, strlen(testIll));
	uut.authFieldDone();

	CHECK(!uut.isAuthorized());
}

TEST(AuthDigest, Strange) {
	const char *testString = "Digest Digest";

	uut.reset("GET");
	uut.parseAuthField(testString, strlen(testString));
	uut.authFieldDone();
	CHECK(!uut.isAuthorized());
}


TEST(AuthDigest, OkThenNok) {
	const char *testString = "Digest "
			"username=\"test\", "
			"realm=\"test\", "
			"nonce=\"verysecurenonce\", "
			"uri=\"/\", "
			"response=\"d20d272dd6ec2d9f135a6c109e71415b\", ";

	uut.reset("GET");
	uut.parseAuthField(testString, strlen(testString));
	uut.authFieldDone();

	CHECK(uut.isAuthorized());

	uut.reset("PUT");
	uut.parseAuthField(testString, strlen(testString));
	uut.authFieldDone();

	CHECK(!uut.isAuthorized());
}
TEST(AuthDigest, AlgorithmMD5) {
	const char *testString = "Digest "
			"username=\"test\", "
			"realm=\"test\", "
			"nonce=\"verysecurenonce\", "
			"uri=\"/\", "
			"response=\"d20d272dd6ec2d9f135a6c109e71415b\", "
			"algorithm=\"MD5\", ";

	uut.reset("GET");
	uut.parseAuthField(testString, strlen(testString));
	uut.authFieldDone();

	CHECK(uut.isAuthorized());
}

TEST(AuthDigest, UnnecessaryRFC2617Fanciness) {
	const char *testString = "Digest "
			"username=\"test\", "
			"realm=\"test\", "
			"nonce=\"verysecurenonce\", "
			"uri=\"/\", "
			"response=\"d20d272dd6ec2d9f135a6c109e71415b\", "
			"algorithm=\"MD5\", "
			"cnonce=\"0687d7d0797409cda85c98ac0834fc65\", "
			"nc=00000001";

	uut.reset("GET");
	uut.parseAuthField(testString, strlen(testString));
	uut.authFieldDone();

	CHECK(uut.isAuthorized());
}

TEST(AuthDigest, WrongUser) {
	const char *testString = "Digest "
			"username=\"god\", "
			"realm=\"test\", "
			"nonce=\"verysecurenonce\", "
			"uri=\"/\", "
			"response=\"d20d272dd6ec2d9f135a6c109e71415b\", ";

	uut.reset("GET");
	uut.parseAuthField(testString, strlen(testString));
	uut.authFieldDone();

	CHECK(!uut.isAuthorized());
}

TEST(AuthDigest, WrongRealm) {
	const char *testString = "Digest "
			"username=\"test\", "
			"realm=\"hell\", "
			"nonce=\"verysecurenonce\", "
			"uri=\"/\", "
			"response=\"d20d272dd6ec2d9f135a6c109e71415b\", ";

	uut.reset("GET");
	uut.parseAuthField(testString, strlen(testString));
	uut.authFieldDone();

	CHECK(!uut.isAuthorized());
}

TEST(AuthDigest, WrongAlgorithm) {
	const char *testString = "Digest "
			"username=\"test\", "
			"realm=\"test\", "
			"nonce=\"verysecurenonce\", "
			"uri=\"/\", "
			"response=\"d20d272dd6ec2d9f135a6c109e71415b\", "
			"algorithm=\"SomethingElse\", ";

	uut.reset("GET");
	uut.parseAuthField(testString, strlen(testString));
	uut.authFieldDone();

	CHECK(!uut.isAuthorized());
}

TEST(AuthDigest, WrongNonce) {
	const char *testString = "Digest "
			"username=\"test\", "
			"realm=\"test\", "
			"nonce=\"spoofed\", "
			"uri=\"/\", "
			"response=\"d20d272dd6ec2d9f135a6c109e71415b\", ";

	uut.reset("GET");
	uut.parseAuthField(testString, strlen(testString));
	uut.authFieldDone();

	CHECK(!uut.isAuthorized());
}

TEST(AuthDigest, WrongUri) {
	const char *testString = "Digest "
			"username=\"test\", "
			"realm=\"test\", "
			"nonce=\"verysecurenonce\", "
			"uri=\"/awesomeness\", "
			"response=\"d20d272dd6ec2d9f135a6c109e71415b\", ";

	uut.reset("GET");
	uut.parseAuthField(testString, strlen(testString));
	uut.authFieldDone();

	CHECK(!uut.isAuthorized());
}

TEST(AuthDigest, WrongMethod) {
	const char *testString = "Digest "
			"username=\"test\", "
			"realm=\"test\", "
			"nonce=\"verysecurenonce\", "
			"uri=\"/\", "
			"response=\"d20d272dd6ec2d9f135a6c109e71415b\", ";

	uut.reset("PUT");
	uut.parseAuthField(testString, strlen(testString));
	uut.authFieldDone();

	CHECK(!uut.isAuthorized());
}

TEST(AuthDigest, WrongResponse) {
	const char *testString = "Digest "
			"username=\"test\", "
			"realm=\"test\", "
			"nonce=\"verysecurenonce\", "
			"uri=\"/\", "
			"response=\"5037814637732\", ";

	uut.reset("GET");
	uut.parseAuthField(testString, strlen(testString));
	uut.authFieldDone();

	CHECK(!uut.isAuthorized());
}

TEST(AuthDigest, Bullshit) {
	const char *testString =
			"\xa9\x5b\x26\xa4\xdb\x41\xcf\x8f\x14\x93\x5b\x2e\xa3\x6e\xf6"
			"\x97\x5e\xa4\xdb\xbc\x7e\xa7\xc7\xf0\xb5\x59\xf1\xa5\x79\x87"
			"\xd4\xcc\xfd\x25\xf8\x2e\x25\x45\x1d\x5c\x99\x70\xd3\xe1\x1f"
			"\xfd\xd2\x59\x71\xe2\x05\x0b\x9e\xd2\xbf\xaf\x5b\xb9\x1e\x57";


	uut.reset("GET");
	uut.parseAuthField(testString, strlen(testString));
	uut.authFieldDone();

	CHECK(!uut.isAuthorized());
}

TEST(AuthDigest, SemiBullshit) {
	const char *testString = "Digest "
			"\xa9\x5b\x26\xa4\xdb\x41\xcf\x8f\x14\x93\x5b\x2e\xa3\x6e\xf6"
			"\x97\x5e\xa4\xdb\xbc\x7e\xa7\xc7\xf0\xb5\x59\xf1\xa5\x79\x87"
			"\xd4\xcc\xfd\x25\xf8\x2e\x25\x45\x1d\x5c\x99\x70\xd3\xe1\x1f"
			"\xfd\xd2\x59\x71\xe2\x05\x0b\x9e\xd2\xbf\xaf\x5b\xb9\x1e\x57";

	uut.reset("GET");
	uut.parseAuthField(testString, strlen(testString));
	uut.authFieldDone();

	CHECK(!uut.isAuthorized());
}
