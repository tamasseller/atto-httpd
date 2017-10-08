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
#ifndef DAVREQUESTPARSER_H_
#define DAVREQUESTPARSER_H_

#include "UXml.h"
#include "TemporaryStringBuffer.h"
#include "Keywords.h"

/**
 * Parser for XML WebDAV requests.
 *
 * It uses the UXml low-level XML parser to convert the input byte
 * stream into an event stream implemented by function calls.
 *
 * Uses the user data storage feature of the UXml parser to store
 * the DAV file and directory properties encountered during the
 * processing of the request. Entity name and namespace pairs are
 * stored next to each other, in this order, separated by a '|'
 * character. The UXml heap is traversed for accessing of the
 * collected properties by the iterator provided.
 */
template<unsigned int stackSize>
class DavRequestParser: UXml<DavRequestParser<stackSize>, stackSize> {
	typedef UXml<DavRequestParser<stackSize>, stackSize> Super;
	friend Super;

public:
	/// WebDAV request type.
	enum class Type: uint8_t {Error, Allprop, Propname, Prop};

private:
	/**
	 * Parser state.
	 *
	 * Each value stands for a level in the expected XML hierarchy.
	 */
	enum class State: uint8_t {Root, Method, Param};

	/*
	 * Standard string constants used in WebDAV requests.
	 */
	static constexpr const char* prop = "prop";
	static constexpr const char* allprop = "allprop";
	static constexpr const char* propname = "propname";
	static constexpr const char* propfind = "propfind";
	static constexpr const char* nsDav = "DAV:";

	/*
	 * UXml parser callback methods.
	 */
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

	/*
	 * Temporary buffer that stores the name of a tag.
	 *
	 * It is needed for later lookup of the DAV file/directory
	 * property.
	 */
	TemporaryStringBuffer<32> temp;

	/// The established type of the request.
	Type type;

	/// State of the request processing.
	State state;

	/// Invalid request error flag.
	bool isBad;
public:

	/// Initialize internal state.
	inline void reset();

	/// Process WebDAV XML request data. Returns false on error.
	inline bool parseDavRequest(const char* buff, uint32_t len);

	/// Finalize processing (should be called when end of data is reached). Returns false on error.
	inline bool done();

	/// Request type accesor.
	inline typename DavRequestParser::Type getType();

	/// Iterator to access the collected properties (for 'prop' requests).
	class PropertyIterator {
		friend DavRequestParser;
		char* current;
		inline PropertyIterator(char* c): current(c) {}
	public:
		/// Validity query, returns false when end of the collection is reached.
		bool isValid();

		/// Property XML element name query, must only be called on a valid iterator.
		void getName(const char* &name, uint32_t &length);
		/// Property XML namespace query, must only be called on a valid iterator.
		void getNs(const char* &ns, uint32_t &length);
	};

	/// Get an iterator instance pointing to the first one.
	inline PropertyIterator propertyIterator();

	/// Move on step in the collection of properties.
	inline bool step(PropertyIterator&);
};

////////////////////////////////////////////////////////////////////////////////


template<unsigned int stackSize>
inline typename DavRequestParser<stackSize>::PropertyIterator DavRequestParser<stackSize>::propertyIterator()
{
	/*
	 * The start of the "heap" is where the element
	 * allocated most recently is, if there is any.
	 */
	char *start = Super::getHeapStart();
	return (start == Super::getHeapEnd()) ? nullptr : start;
}

template<unsigned int stackSize>
bool inline DavRequestParser<stackSize>::step(PropertyIterator& it)
{
	if(!it.isValid())
		return false;

	// Find the end of the current string.
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
