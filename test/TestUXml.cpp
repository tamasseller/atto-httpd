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

#include "1test/Test.h"
#include "1test/Mock.h"

#include "UXml.h"

#include <string.h>

#include <list>
#include <string>
#include <functional>

#include <iostream> //XXX

namespace {
	struct Uut: public UXml<Uut, 192> {
		std::string name;
		std::function<void (std::string)> onContentStartEvent = nullptr;

		inline void onContent(const char* buff, uint32_t len) {

		}

		inline void onContentStart() {
			if(onContentStartEvent)
				onContentStartEvent(name);
		}


		inline void onTagStart() {
			MOCK(uxml)::CALL(TagStart);
			name.clear();
		}

		inline void onTagName(const char* buff, uint32_t len) {
			name += std::string(buff, len);
		}

		inline void onTagNameEnd() {
			MOCK(uxml)::CALL(TagNameEnd).withStringParam(name.c_str());
		}

		inline void onAttributeNameStart() {
			MOCK(uxml)::CALL(AttrNameStart);
			name.clear();
		}

		inline void onAttributeName(const char* buff, uint32_t len) {
			name += std::string(buff, len);
		}

		inline void onAttributeNameEnd() {
			MOCK(uxml)::CALL(AttrNameEnd).withStringParam(name.c_str());
		}

		inline void onAttributeValueStart() {
			MOCK(uxml)::CALL(AttrValueStart);
			name.clear();
		}

		inline void onAttributeValue(const char* buff, uint32_t len) {
			name += std::string(buff, len);
		}

		inline void onAttributeValueEnd() {
			MOCK(uxml)::CALL(AttrValueEnd).withStringParam(name.c_str());
		}

		inline void onCloseTag() {
			MOCK(uxml)::CALL(CloseTag);
			name.clear();
		}
	};
}

class UXmlStackTest {
	Uut uut;
	template<class... T>
	void expect(T... raw) {
		const char* strs[] = {raw...};

		uint32_t idx;
		idx = uut.stackIdx;

		for(uint32_t i=0; i<sizeof...(T); i++) {
			idx = uut.findPrevious(idx );
			CHECK(strncmp(uut.stack+idx, strs[i], strlen(strs[i])) == 0);
		}

		CHECK(!idx);
	}

	void writeString(const char* str) {
		CHECK(uut.writeEntry(str, strlen(str)));
		CHECK(uut.terminateEntry());
	}
public:
	void runBasicMatcherTest() {
		uut.reset();

		CHECK(!uut.resolve("whatever"));

		writeString("foo=bar");

		CHECK(strcmp(uut.resolve("foo"), "bar") == 0);
		CHECK(strcmp(uut.resolve("foogarbage", 3), "bar") == 0);

		writeString("foobar=baz");

		CHECK(!uut.resolve("whatever"));
		CHECK(strcmp(uut.resolve("foo"), "bar") == 0);
		CHECK(strcmp(uut.resolve("foobar"), "baz") == 0);

		writeString("foo=override");

		CHECK(strcmp(uut.resolve("foo"), "override") == 0);

		writeString("=x");
		writeString("y=");

		CHECK(strcmp(uut.resolve(""), "x") == 0);
		CHECK(strcmp(uut.resolve("y"), "") == 0);
	}


	void runMatcherNestingTest() {
		uut.reset();

		/*
		 *  Add two items to the first frame
		 */
		writeString("foo=bar");
		writeString("xy=z");

		CHECK(uut.terminateFrame());

		CHECK(strcmp(uut.resolve("foo"), "bar") == 0);
		CHECK(strcmp(uut.resolve("xy"), "z") == 0);
		expect("", "xy=z", "foo=bar");

		/*
		 * Add two to the next one of which that shadows an entry from the previous frame
		 */
		writeString("abc=def");

		CHECK(strcmp(uut.resolve("foo"), "bar") == 0);
		CHECK(strcmp(uut.resolve("xy"), "z") == 0);
		CHECK(strcmp(uut.resolve("abc"), "def") == 0);

		writeString("foo=baz");

		CHECK(strcmp(uut.resolve("foo"), "baz") == 0);
		CHECK(strcmp(uut.resolve("xy"), "z") == 0);
		CHECK(strcmp(uut.resolve("abc"), "def") == 0);

		CHECK(uut.terminateFrame());

		CHECK(strcmp(uut.resolve("foo"), "baz") == 0);
		CHECK(strcmp(uut.resolve("xy"), "z") == 0);
		CHECK(strcmp(uut.resolve("abc"), "def") == 0);
		expect("", "foo=baz", "abc=def", "", "xy=z", "foo=bar");

		/*
		 * Drop the last frame
		 */
		CHECK(uut.dropStackFrame());

		CHECK(strcmp(uut.resolve("foo"), "bar") == 0);
		CHECK(strcmp(uut.resolve("xy"), "z") == 0);
		CHECK(!uut.resolve("abc"));
		expect("", "xy=z", "foo=bar");

		/*
		 * Add a new entry
		 */
		writeString("new=entry");

		CHECK(strcmp(uut.resolve("foo"), "bar") == 0);
		CHECK(strcmp(uut.resolve("xy"), "z") == 0);
		CHECK(strcmp(uut.resolve("new"), "entry") == 0);
		expect("new=entry", "", "xy=z", "foo=bar");

		/*
		 * Drop it
		 */

		CHECK(uut.dropStackFrame());

		CHECK(strcmp(uut.resolve("foo"), "bar") == 0);
		CHECK(strcmp(uut.resolve("xy"), "z") == 0);
		CHECK(!uut.resolve("new"));
		expect("", "xy=z", "foo=bar");

		/*
		 * Add empty frame, then drop it
		 */

		CHECK(uut.terminateFrame());
		expect("", "", "xy=z", "foo=bar");

		CHECK(uut.dropStackFrame());
		expect("", "xy=z", "foo=bar");

		/*
		 * Drop the last one
		 */

		CHECK(uut.dropStackFrame());

		CHECK(!uut.resolve("xy"));
		CHECK(!uut.resolve("foo"));
		expect();

		/*
		 * Should be empty
		 */
		CHECK(!uut.dropStackFrame());
	}

	void runOverfillTest() {
		uut.reset();

		for(int i=0; i<192; i++)
			CHECK(uut.writeEntry("x", 1));

		CHECK(!uut.writeEntry("x", 1));
		CHECK(!uut.terminateEntry());
		CHECK(!uut.terminateFrame());
	}

	void runOverallocTest() {
		uut.reset();

		for(int i=0; i<11; i++)
			CHECK(uut.alloc(16));

		CHECK(!uut.alloc(16));
	}

	void runMixedOomTest() {
		uut.reset();

		for(int i=0; i<5; i++) {
			CHECK(uut.writeEntry("0123456789abcde", 15));
			CHECK(uut.terminateEntry());
			CHECK(uut.alloc(16));
		}

		CHECK(uut.alloc(16));
		CHECK(uut.writeEntry("0123456789abcde", 15));
		CHECK(uut.terminateEntry());
		CHECK(!uut.writeEntry('x'));
		CHECK(!uut.alloc(16));
	}
};

TEST_GROUP(UXml) {
	Uut uut;

	void expectTag(const char* name) {
		MOCK(uxml)::EXPECT(TagStart);
		MOCK(uxml)::EXPECT(TagNameEnd).withStringParam(name);
	}

	void expectAttribute(const char* name, const char* value) {
		MOCK(uxml)::EXPECT(AttrNameStart);
		MOCK(uxml)::EXPECT(AttrNameEnd).withStringParam(name);
		MOCK(uxml)::EXPECT(AttrValueStart);
		MOCK(uxml)::EXPECT(AttrValueEnd).withStringParam(value);
	}

	void expectClose() {
		MOCK(uxml)::EXPECT(CloseTag);
	}

};

TEST(UXml, MatcherBasic) {
	UXmlStackTest().runBasicMatcherTest();
}

TEST(UXml, MatcherNesting) {
	UXmlStackTest().runMatcherNestingTest();
}

TEST(UXml, StackOverflow) {
	UXmlStackTest().runOverfillTest();
}

TEST(UXml, HeapOverflow) {
	UXmlStackTest().runOverallocTest();
}

TEST(UXml, MixedOom) {
	UXmlStackTest().runMixedOomTest();
}

TEST(UXml, Sanity) {
	constexpr const char* input = "<!DOCTYPE haxml><tag attr = 'val'></tag>";

	for(uint32_t i=0; i<strlen(input); i++) {
		expectTag("tag");
		expectAttribute("attr", "val");
		expectClose();

		uut.reset();
		CHECK(uut.parseXml(input, i));
		CHECK(uut.parseXml(input + i, strlen(input) - i));
		CHECK(uut.done());
	}
}

TEST(UXml, Pi) {
	constexpr const char* input = "<?pi whatever ?>";

	for(uint32_t i=0; i<strlen(input); i++) {
		uut.reset();
		CHECK(uut.parseXml(input, i));
		CHECK(uut.parseXml(input + i, strlen(input) - i));
		CHECK(uut.done());
	}
}


TEST(UXml, SelfClose) {
	constexpr const char* input = "<a/>";

	for(uint32_t i=0; i<strlen(input); i++) {
		expectTag("a");
		expectClose();

		uut.reset();
		CHECK(uut.parseXml(input, i));
		CHECK(uut.parseXml(input + i, strlen(input) - i));
		CHECK(uut.done());
	}
}

TEST(UXml, TwoLevelSelfClose) {
	constexpr const char* input = "<a><b/></a>";

	for(uint32_t i=0; i<strlen(input); i++) {
		expectTag("a");
		expectTag("b");
		expectClose();
		expectClose();

		uut.reset();
		CHECK(uut.parseXml(input, i));
		CHECK(uut.parseXml(input + i, strlen(input) - i));
		CHECK(uut.done());
	}
}

TEST(UXml, TwoLevelSelfCloseChildren) {
	constexpr const char* input = "<a><b/><c></c><d/></a>";

	for(uint32_t i=0; i<strlen(input); i++) {
		expectTag("a");

		expectTag("b");
		expectClose();

		expectTag("c");
		expectClose();

		expectTag("d");
		expectClose();

		expectClose();

		uut.reset();
		CHECK(uut.parseXml(input, i));
		CHECK(uut.parseXml(input + i, strlen(input) - i));
		CHECK(uut.done());
	}
}

TEST(UXml, TwoLevelSelfCloseChildrenAttrs) {
	constexpr const char* input = "<a foo ='bar'>\n\t<b frob= 'bork'/>\n\t<c x = \"y\" u='v'></c>\n\t<d/></a>";

	for(uint32_t i=0; i<strlen(input); i++) {
		expectTag("a");
		expectAttribute("foo", "bar");

		expectTag("b");
		expectAttribute("frob", "bork");
		expectClose();

		expectTag("c");
		expectAttribute("x", "y");
		expectAttribute("u", "v");

		expectClose();

		expectTag("d");
		expectClose();

		expectClose();

		uut.reset();
		CHECK(uut.parseXml(input, i));
		CHECK(uut.parseXml(input + i, strlen(input) - i));
		CHECK(uut.done());
	}
}

TEST(UXml, FunnyAttributeQuotes) {
	constexpr const char* input = "<a foo='\"bar\"' bar=\"'foo'\"/>";

	for(uint32_t i=0; i<strlen(input); i++) {
		expectTag("a");
		expectAttribute("foo", "\"bar\"");
		expectAttribute("bar", "'foo'");
		expectClose();

		uut.reset();
		CHECK(uut.parseXml(input, i));
		CHECK(uut.parseXml(input + i, strlen(input) - i));
		CHECK(uut.done());
	}
}

TEST(UXml, AlmostNamespace) {
	constexpr const char* input = "<a x='' xx='' "
									 "xm='' xmm='' "
									 "xml='' xmll='' "
									 "xmln='' xmlnn='' "
									 "xmlnonsense='' xmlnstuff=''/>";

	for(uint32_t i=0; i<strlen(input); i++) {
		expectTag("a");
		expectAttribute("x", "");
		expectAttribute("xx", "");
		expectAttribute("xm", "");
		expectAttribute("xmm", "");
		expectAttribute("xml", "");
		expectAttribute("xmll", "");
		expectAttribute("xmln", "");
		expectAttribute("xmlnn", "");
		expectAttribute("xmlnonsense", "");
		expectAttribute("xmlnstuff", "");
		expectClose();

		uut.reset();
		CHECK(uut.parseXml(input, i));
		CHECK(uut.parseXml(input + i, strlen(input) - i));
		CHECK(uut.done());
	}
}

TEST(UXml, NamespaceParse) {
	constexpr const char* input = "<a xmlns='foo' xmlns:bar='baz'/>";

	for(uint32_t i=0; i<strlen(input); i++) {
		expectTag("a");
		expectClose();

		uut.onContentStartEvent = [&](std::string name){
			CHECK(std::string(uut.resolve("")) == "foo");
			CHECK(std::string(uut.resolve("bar")) == "baz");
		};

		uut.reset();
		CHECK(uut.parseXml(input, i));
		CHECK(uut.parseXml(input + i, strlen(input) - i));
		CHECK(uut.done());
	}
}

TEST(UXml, NamespaceContext) {
	constexpr const char* input = "<a xmlns =\"ns1\" xmlns:foo = 'ns2'><b/><c xmlns = 'ns3' xmlns:bar='ns4'/></a>";

	for(uint32_t i=0; i<strlen(input); i++) {
		expectTag("a");
		expectTag("b");
		expectTag("c");
		expectClose();
		expectClose();
		expectClose();

		uut.onContentStartEvent = [&](std::string name){
			if(name == "a") {
				CHECK(std::string(uut.resolve("")) == "ns1");
				CHECK(std::string(uut.resolve("foo")) == "ns2");
				CHECK(!uut.resolve("bar"));
			} else if(name == "c") {
				CHECK(std::string(uut.resolve("")) == "ns3");
				CHECK(std::string(uut.resolve("foo")) == "ns2");
				CHECK(std::string(uut.resolve("bar")) == "ns4");
			}
		};

		uut.reset();
		CHECK(uut.parseXml(input, i));
		CHECK(uut.parseXml(input + i, strlen(input) - i));
		CHECK(uut.done());
	}
}

TEST(UXml, NsOverload) {
	constexpr const char* input = "<a xmlns:a='bcde'>";

	MOCK(uxml)::disable();

	uut.reset();

	for(uint32_t i=0; i<24; i++)
		CHECK(uut.parseXml(input, strlen(input)));

	CHECK(!uut.parseXml(input, strlen(input)));
}

TEST(UXml, NsOverload2) {
	constexpr const char* input = "<a xmlns:a='bcde'>";

	MOCK(uxml)::disable();

	uut.reset();

	for(uint32_t i=0; i<24; i++)
		CHECK(uut.parseXml(input, strlen(input)));

	constexpr const char* finishHim = "<a xmlns=''/>";
	CHECK(!uut.parseXml(finishHim , strlen(finishHim)));
}


TEST(UXml, NsOverload3) {
	constexpr const char* input = "<a xmlns:a='bcdef'>";

	MOCK(uxml)::disable();

	uut.reset();

	for(uint32_t i=0; i<21; i++)
		CHECK(uut.parseXml(input, strlen(input)));

	CHECK(!uut.parseXml(input, strlen(input)));
}

TEST(UXml, RealDavRequest) {
	constexpr const char* input =
			"<?xml version=\"1.0\" encoding=\"utf-8\" ?>\n"
			"<D:propertyupdate xmlns:D=\"DAV:\">\n"
			"	<D:remove>"
			"		<D:prop>"
			"			<executable xmlns=\"http://webdav.org/cadaver/custom-properties/\"></executable>"
			"		</D:prop>"
			"	</D:remove>\n"
			"</D:propertyupdate>";


	std::list<std::string> expected{"DAV:propertyupdate",
									"DAV:remove",
									"DAV:prop",
									"http://webdav.org/cadaver/custom-properties/executable"};

	std::list<std::string> tags;
	uut.onContentStartEvent = [&](std::string name) {
		std::string label, tagName;
		auto idx = name.find(":");

		if(idx != std::string::npos) {
			label = name.substr(0, idx);
			tagName = name.substr(idx+1);
		} else {
			label = "";
			tagName = name;
		}

		const char* rns = uut.resolve(label.c_str());
		std::string ns = (rns) ? rns : "--- default ---";

		tags.push_back(ns + tagName);
	};


	for(uint32_t i=0; i<strlen(input); i++) {
		tags.clear();
		expectTag("D:propertyupdate");
		expectTag("D:remove");
		expectTag("D:prop");
		expectTag("executable");
		expectClose();
		expectClose();
		expectClose();
		expectClose();

		uut.reset();
		CHECK(uut.parseXml(input, i));
		CHECK(uut.parseXml(input + i, strlen(input) - i));
		CHECK(uut.done());

		CHECK(tags == expected);
	}
}


TEST(UXml, RealDavRequestWithSave) {
	constexpr const char* input =
			"<?xml version=\"1.0\" encoding=\"utf-8\" ?>\n"
			"<D:propertyupdate xmlns:D=\"DAV:\">\n"
			"	<D:remove>"
			"		<D:prop>"
			"			<executable xmlns=\"http://webdav.org/cadaver/custom-properties/\"></executable>"
			"		</D:prop>"
			"	</D:remove>\n"
			"</D:propertyupdate>";

	std::list<std::string> expected{"executable|http://webdav.org/cadaver/custom-properties/",
									"prop|DAV:",
									"remove|DAV:",
									"propertyupdate|DAV:"};

	char* last;
	uut.onContentStartEvent = [&](std::string name) {
		std::string label, tagName;
		auto idx = name.find(":");

		if(idx != std::string::npos) {
			label = name.substr(0, idx);
			tagName = name.substr(idx+1);
		} else {
			label = "";
			tagName = name;
		}

		const char* rns = uut.resolve(label.c_str());
		std::string ns = (rns) ? rns : "--- default ---";
		std::string entry = tagName + "|" + ns;

		last = uut.alloc(entry.length()+1);
		CHECK(last);
		strcpy(last, entry.c_str());
	};

	MOCK(uxml)::disable();
	for(uint32_t i=0; i<strlen(input); i++) {
		uut.reset();
		CHECK(uut.parseXml(input, i));
		CHECK(uut.parseXml(input + i, strlen(input) - i));
		CHECK(uut.done());

		for(auto x: expected) {
			CHECK(strcmp(last, x.c_str()) == 0);
			last += x.length()+1;
		}
	}
	MOCK(uxml)::enable();
}

