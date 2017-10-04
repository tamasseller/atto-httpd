/*
 * BaseUJsonTest.h
 *
 *  Created on: 2017.10.04.
 *      Author: tooma
 */

#ifndef BASEUJSONTEST_H_
#define BASEUJSONTEST_H_

#include "1test/Test.h"
#include "1test/Mock.h"

#include "UJson.h"

#include <string>
#include <string.h>

// TODO test root value without object.


TEST_GROUP(UJson) {
	struct Uut: public UJson<Uut, 32> {
		std::string key, value;
		bool hasType = false;
		JsonValueType lastType;

		inline void beforeKey() {
			key = "";
		}
		inline void onKey(const char *at, size_t length) {
			key += std::string(at, length);
		}
		inline void afterKey() {
			MOCK(ujson)::CALL(key).withStringParam(key.c_str());}

		inline void beforeValue(JsonValueType type) {
			CHECK(!hasType);

			if(type == JsonValueType::String)
				value = "";

			if (type == JsonValueType::Array) {
				MOCK(ujson)::CALL(enterArray);
				hasType = false;
			} else if (type == JsonValueType::Object) {
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
		inline void afterValue(JsonValueType type) {
			if(type == JsonValueType::String)
				MOCK(ujson)::CALL(string).withStringParam(value.c_str());
			else if (type == JsonValueType::Array)
				MOCK(ujson)::CALL(leaveArray);
			else if (type == JsonValueType::Object)
				MOCK(ujson)::CALL(leaveObject);

			if(hasType)
				CHECK(type == lastType);

			hasType = false;
		}

		inline void onKeyError() {MOCK(ujson)::CALL(keyError);}
		inline void onValueError() {MOCK(ujson)::CALL(valueError);}
		inline void onStructureError() {MOCK(ujson)::CALL(structureError);}
		inline void onResourceError() {MOCK(ujson)::CALL(resourceError);}
	} uut;

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

	void expectKeyError() {
		MOCK(ujson)::EXPECT(keyError);
	}

	void expectValueError() {
		MOCK(ujson)::EXPECT(valueError);
	}

    void expectStructureError() {
    	MOCK(ujson)::EXPECT(structureError);
    }

    void expectResourceError() {
    	MOCK(ujson)::EXPECT(resourceError);
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

#endif /* BASEUJSONTEST_H_ */
