/*
 * DavRequestParser.h
 *
 *  Created on: 2017.02.12.
 *      Author: tooma
 */

#ifndef DAVREQUESTPARSER_H_
#define DAVREQUESTPARSER_H_

#include "UXml.h"
#include "TemporaryStringBuffer.h"
#include "Keywords.h"

template<unsigned int stackSize>
class DavRequestParser: UXml<DavRequestParser<stackSize>, stackSize> {
	typedef UXml<DavRequestParser<stackSize>, stackSize> Super;
	friend Super;

public:
	enum class Type: uint8_t {Error, Allprop, Propname, Prop};

private:
	enum class State: uint8_t {Root, Method, Param};

	static constexpr const char* prop = "prop";
	static constexpr const char* allprop = "allprop";
	static constexpr const char* propname = "propname";
	static constexpr const char* propfind = "propfind";
	static constexpr const char* nsDav = "DAV:";

	inline void onContent(const char* buff, uint32_t len);
	inline void onContentStart();
	inline void onTagStart();
	inline void onTagName(const char* buff, uint32_t len);
	inline void onTagNameEnd();
	inline void onAttributeNameStart();
	inline void onAttributeName(const char* buff, uint32_t len);
	inline void onAttributeNameEnd();
	inline void onAttributeValueStart();
	inline void onAttributeValue(const char* buff, uint32_t len);
	inline void onAttributeValueEnd();
	inline void onCloseTag();

	TemporaryStringBuffer<32> temp;
	Type type;
	State state;
	bool isBad;
public:
	inline void reset();
	inline bool parseDavRequest(const char* buff, uint32_t len);
	inline bool done();
	inline typename DavRequestParser::Type getType();

	class PropertyIterator {
		friend DavRequestParser;
		char* current;
		inline PropertyIterator(char* c): current(c) {}
	public:
		bool isValid();
		void getName(const char* &name, uint32_t &length);
		void getNs(const char* &ns, uint32_t &length);
	};

	inline PropertyIterator propertyIterator();
	inline bool step(PropertyIterator&);
};

////////////////////////////////////////////////////////////////////////////////


template<unsigned int stackSize>
inline typename DavRequestParser<stackSize>::PropertyIterator DavRequestParser<stackSize>::propertyIterator()
{
	char *start = Super::getHeapStart();
	return (start == Super::getHeapEnd()) ? nullptr : start;
}

template<unsigned int stackSize>
bool inline DavRequestParser<stackSize>::step(PropertyIterator& it)
{
	if(!it.isValid())
		return false;

	while(*it.current++);

	if(it.current == Super::getHeapEnd()) {
		it.current = nullptr;
		return true;
	}

	return it.current != Super::getHeapEnd();
}

template<unsigned int stackSize>
inline void DavRequestParser<stackSize>::PropertyIterator::getName(const char* &name, uint32_t &length)
{
	name = current;
	for(length=0; name[length] != '|'; length++);
}

template<unsigned int stackSize>
inline void DavRequestParser<stackSize>::PropertyIterator::getNs(const char* &ns, uint32_t &length)
{
	for(ns=current; *ns++ != '|';);
	for(length=0; ns[length]; length++);
}

template<unsigned int stackSize>
inline bool DavRequestParser<stackSize>::PropertyIterator::isValid()
{
	return current != nullptr;
}

template<unsigned int stackSize>
inline void DavRequestParser<stackSize>::onTagStart()
{
	temp.clear();
}

template<unsigned int stackSize>
inline void DavRequestParser<stackSize>::onTagName(const char* buff, uint32_t len)
{
	if(!temp.save(buff, len))
		isBad = true;
}

template<unsigned int stackSize>
inline void DavRequestParser<stackSize>::onContentStart()
{
	int colonIdx = -1;
	temp.terminate();
	const char* data = temp.data();

	for(int i=0; data[i]; i++) {
		if(data[i] == ':') {
			colonIdx = i;
			break;
		}
	}

	const char *ns, *const name = data + colonIdx + 1;
	if(colonIdx != -1)
		ns = Super::resolve(data, colonIdx);
	else
		ns = Super::resolve("");

	const uint32_t nameLength = strlen(name);
	const uint32_t nsLength = ns ? strlen(ns) : 0;

	switch(state) {
		case State::Root:
			if(strcmp(name, propfind) == 0) {
				state = State::Method;
			} else
				isBad = true;

			break;
		case State::Method:
			if(strcmp(name, prop) == 0) {
				state = State::Param;
				type = Type::Prop;
			} else if(strcmp(name, propname) == 0) {
				type = Type::Propname;
			} else if(strcmp(name, allprop) == 0) {
				type = Type::Allprop;
			} else
				isBad = true;
			break;
		default: // just to keep branch coverage analyzer happy
		case State::Param: {
			const uint32_t storageSize = nameLength + nsLength + 2;
			char *storage = Super::alloc(storageSize);

			if(storage) {
				memcpy(storage, data + (colonIdx + 1), nameLength);
				storage[nameLength] = '|';
				memcpy(storage + nameLength + 1, ns, nsLength);
				storage[storageSize - 1] = '\0';
			} else
				isBad = true;

		} break;
	}
}

template<unsigned int stackSize>
inline void DavRequestParser<stackSize>::onContent(const char* buff, uint32_t len) {}

template<unsigned int stackSize>
inline void DavRequestParser<stackSize>::onTagNameEnd() {}

template<unsigned int stackSize>
inline void DavRequestParser<stackSize>::onCloseTag() {}

template<unsigned int stackSize>
inline void DavRequestParser<stackSize>::onAttributeNameStart() {}

template<unsigned int stackSize>
inline void DavRequestParser<stackSize>::onAttributeName(const char* buff, uint32_t len) {}

template<unsigned int stackSize>
inline void DavRequestParser<stackSize>::onAttributeNameEnd() {}

template<unsigned int stackSize>
inline void DavRequestParser<stackSize>::onAttributeValueStart() {}

template<unsigned int stackSize>
inline void DavRequestParser<stackSize>::onAttributeValue(const char* buff, uint32_t len) {}

template<unsigned int stackSize>
inline void DavRequestParser<stackSize>::onAttributeValueEnd() {}

template<unsigned int stackSize>
inline typename DavRequestParser<stackSize>::Type DavRequestParser<stackSize>::getType()
{
	return type;
}

template<unsigned int stackSize>
inline bool DavRequestParser<stackSize>::parseDavRequest(const char* buff, uint32_t len)
{
	if(isBad || !Super::parseXml(buff, len))
		isBad = true;

	return !isBad;
}

template<unsigned int stackSize>
inline void DavRequestParser<stackSize>::reset()
{
	state = State::Root;
	type = Type::Allprop;
	isBad = false;
	Super::reset();
}

template<unsigned int stackSize>
inline bool DavRequestParser<stackSize>::done()
{
	if(isBad || !Super::done())
		isBad = true;

	return !isBad;
}

#endif /* DAVREQUESTPARSER_H_ */
