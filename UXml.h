/*
 * UXml.h
 *
 *  Created on: 2017.02.13.
 *      Author: tooma
 */

#ifndef UXML_H_
#define UXML_H_

#include <stdint.h>
#include <string.h>

template<class Child, unsigned int stackSize = 128>
class UXml {
	static constexpr const char *xmlns="xmlns";

	enum class State: uint8_t {
		Content,
		TagStart,
		TagName,
		Attrs,
		AttrNs_X,
		AttrNs_XM,
		AttrNs_XML,
		AttrNs_XMLN,
		AttrNs_XMLNS,
		AttrNsPrefix,
		AttrNsQuot,
		AttrNsUrl,
		AttrName,
		AttrValQuot,
		AttrVal,
		WaitClose
	};

	uint16_t stackIdx, stackEnd;
	char stack[stackSize];
	State state;
	char delimiter;

	friend class UXmlStackTest;
	inline bool writeEntry(char);
	inline bool writeEntry(const char *, uint32_t);
	inline bool terminateEntry();
	inline bool terminateFrame();
	inline bool dropStackFrame();
	inline int findPrevious(int);

protected:
	inline char* getHeapStart();
	inline char* getHeapEnd();

public:
	inline char* alloc(uint32_t size);
	inline const char* resolve(const char*, uint32_t length=-1u);
	inline void reset();
	inline bool parseXml(const char* buff, uint32_t length);
	inline bool done();
};

/*
 * Writes the next character(s) of an entry
 *
 * ENTRIES MUST BE SPECIFIED AS <key>=<value> !
 *
 * Where both the key and the value can be empty string.
 * The entry can be terminated with terminate entry.
 */
template<class Child, unsigned int stackSize>
inline bool UXml<Child, stackSize>::writeEntry(const char *c, uint32_t length)
{
	if(stackIdx + length > stackEnd)
		return false;

	memcpy(stack + stackIdx, c, length);
	stackIdx += length;
	return true;
};

template<class Child, unsigned int stackSize>
inline bool UXml<Child, stackSize>::writeEntry(char c)
{
	if(stackIdx < stackEnd) {
		stack[stackIdx++] = c;
		return true;
	}

	return false;
};

/*
 *	The entry is terminated with a zero byte.
 */
template<class Child, unsigned int stackSize>
inline bool UXml<Child, stackSize>::terminateEntry()
{
	return writeEntry('\0');
};

/*
 *	The frame is terminated with a zero byte too.
 *
 *	MUST NOT BE CALLED WITH AN UNTERMINATED ENTRY.
 *
 *	The actual terminator of the frame is an empty
 *	string entry, so that is why it has to called
 *	with a terminated entry.
 */
template<class Child, unsigned int stackSize>
inline bool UXml<Child, stackSize>::terminateFrame()
{
	// assert(!stack[stackIdx]);

	return writeEntry('\0');
};

/*
 * Find the start of the first string backwards
 * from the specified index (returns -1 on error
 * as 0 is a completely valid starting address
 * index for the first stacked item).
 */
template<class Child, unsigned int stackSize>
inline int UXml<Child, stackSize>::findPrevious(int x)
{
	if(!x)
		return -1;

	while(--x) {
		if(!stack[x-1])
			return x;
	}

	return x;
}

/*
 * Deletes the last frame, keeps the previous
 * empty terminator entry.
 */
template<class Child, unsigned int stackSize>
inline bool UXml<Child, stackSize>::dropStackFrame()
{
	if(!stackIdx)
		return false;

	bool lastWasZero = false;

	while(--stackIdx) {

		if(!stack[stackIdx-1]) {
			if(lastWasZero) {
				stackIdx++;
				break;
			}

			lastWasZero = true;
		} else
			lastWasZero = false;
	}

	return true;
}

/*
 * Matches the name part only and returns the value part
 */
template<class Child, unsigned int stackSize>
inline const char* UXml<Child, stackSize>::resolve(const char* str, uint32_t length)
{
	for(uint32_t idx = findPrevious(stackIdx); idx != -1u; idx = findPrevious(idx)) {
		uint32_t j;
		for(j = 0; j < length && str[j]; j++)
			if(stack[idx+j] != str[j])
				break;

		if((!str[j] || j == length) && stack[idx+j] == '=')
			return stack+idx+j+1;
	}

	return nullptr;
}

/*
 * Allocates a block from the end of the stack.
 */
template<class Child, unsigned int stackSize>
inline char* UXml<Child, stackSize>::alloc(uint32_t size)
{
	if(stackIdx >= stackEnd - size)
		return nullptr;

	stackEnd -= size;

	return stack + stackEnd;
}

/*
 * Returns the start of the space that is allocated for user
 * data blocks in the end of the stack (ie. heap)
 */
template<class Child, unsigned int stackSize>
inline char* UXml<Child, stackSize>::getHeapStart()
{
	return stack + stackEnd;
}

/*
 * Returns the end of the space that is allocated for user
 * data blocks in the end of the stack (ie. heap)
 */
template<class Child, unsigned int stackSize>
inline char* UXml<Child, stackSize>::getHeapEnd()
{
	return stack + stackSize;
}


/*
 * Resets the internal state to initial values.
 */
template<class Child, unsigned int stackSize>
inline void UXml<Child, stackSize>::reset()
{
	stackIdx = 0;
	stackEnd = stackSize;
	state = State::Content;
}

/*
 * Parses a block of input, it can be arbitrarily segmented.
 */
template<class Child, unsigned int stackSize>
inline bool UXml<Child, stackSize>::parseXml(const char* buff, uint32_t length)
{
	const char* const end = buff + length;
	const char* start;

	while(buff != end) {
		switch(state) {
		/*
		 *  The default case is never taken, but have been put
		 *  here to keep lcov branch coverage analyzer happy.
		 */
		default:

		/*
		 * Processes the content of an element and garbage outside of the root.
		 */
		case State::Content:
			start = buff;
			while(buff != end && *buff != '<')
				buff++;

			((Child*)this)->onContent(start, buff-start);

			if(buff == end)
				return true;

			buff++;
			state = State::TagStart;

		/*
		 * Entered with the data pointer being at the first character after
		 * the initial <. Branches depending on the next character.
		 * There is no break, to allow fall-through in the common case.
		 */
		case State::TagStart:
			if(buff == end)
				return true;

			if(*buff == '/') {
				((Child*)this)->onCloseTag();
				dropStackFrame();
			}

			if(*buff == '?' || *buff == '!' || *buff == '/') {
				state = State::WaitClose;
				buff++;
				break;
			}

			((Child*)this)->onTagStart();

			state = State::TagName;

		/*
		 * Processes the name of the tag (foo for a <foo>)
		 * There is no break, to allow fall-through in the common case.
		 */
		case State::TagName:
			start = buff;

			while(buff != end && *buff != ' ' && *buff != '/' && *buff != '>')
				buff++;

			((Child*)this)->onTagName(start, buff-start);

			if(buff == end)
				return true;
			else
				((Child*)this)->onTagNameEnd();

			state = State::Attrs;

		/*
		 * Processes the whitespace between the attributes of an element.
		 * There is no break, to allow fall-through in the common case.
		 */
		case State::Attrs:
			while(buff != end && *buff == ' ')
				buff++;

			if(buff == end)
				return true;
			else {
				if(*buff == '/') {
					((Child*)this)->onContentStart();
					((Child*)this)->onCloseTag();

					terminateFrame();
					dropStackFrame();

					state = State::WaitClose;
					buff++;
					break;
				} else if(*buff == '>') {
					if(!terminateFrame())
						return false;

					((Child*)this)->onContentStart();
					state = State::Content;
					buff++;
					break;
				} else {
					if(*buff != 'x') {
						((Child*)this)->onAttributeNameStart();
						state = State::AttrName;
						break;
					} else {
						state = State::AttrNs_X;
						buff++;
					}
				}
			}

		/*
		 * These states are used to processes the namespace attribute
		 * name (xmlns). The state information used for the string
		 * matching is collapsed into the state variable this way.
		 * There is no break, to allow fall-through in the common case.
		 */
		case State::AttrNs_X:
			if(buff == end)
				return true;
			else if(*buff == 'm') {
				state = State::AttrNs_XM;
				buff++;
			} else {
				((Child*)this)->onAttributeNameStart();
				((Child*)this)->onAttributeName(xmlns, 1);
				state = State::AttrName;
				break;
			}

		/* There is no break, to allow fall-through in the common case. */
		case State::AttrNs_XM:
			if(buff == end)
				return true;
			else if(*buff == 'l') {
				state = State::AttrNs_XML;
				buff++;
			} else {
				((Child*)this)->onAttributeNameStart();
				((Child*)this)->onAttributeName(xmlns, 2);
				state = State::AttrName;
				break;
			}

		/* There is no break, to allow fall-through in the common case. */
		case State::AttrNs_XML:
			if(buff == end)
				return true;
			else if(*buff == 'n') {
				state = State::AttrNs_XMLN;
				buff++;
			} else {
				((Child*)this)->onAttributeNameStart();
				((Child*)this)->onAttributeName(xmlns, 3);
				state = State::AttrName;
				break;
			}

		/* There is no break, to allow fall-through in the common case. */
		case State::AttrNs_XMLN:
			if(buff == end)
				return true;
			else if(*buff == 's') {
				state = State::AttrNs_XMLNS;
				buff++;
			} else {
				((Child*)this)->onAttributeNameStart();
				((Child*)this)->onAttributeName(xmlns, 4);
				state = State::AttrName;
				break;
			}

		/*
		 * Here the matching of the literal 'xmlns' part is done.
		 * Branches off for no prefix, falls through otherwise.
		 * There is no break, to allow fall-through in the common case.
		 */
		case State::AttrNs_XMLNS:
			if(buff == end)
				return true;
			else if(*buff == ':') {
				state = State::AttrNsPrefix;
				buff++;
			} else if(*buff == '=' || *buff == ' ') {
				state = State::AttrNsQuot;
				buff++;
				break;
			} else {
				((Child*)this)->onAttributeNameStart();
				((Child*)this)->onAttributeName(xmlns, 5);
				state = State::AttrName;
				break;
			}

		/*
		 * Processes the namespace definition prefix part (which is
		 * actually a suffix in the definition) of the attribute.
		 * There is no break, to allow fall-through in the common case.
		 */
		case State::AttrNsPrefix:
			start = buff;

			while(buff != end && *buff != ' ' && *buff != '=')
				buff++;

			if(!writeEntry(start, buff - start))
				return false;

			if(buff == end)
				return true;

			state = State::AttrNsQuot;

		/*
		 * Consumes whitespace and equals sign before the URL value.
		 * There is no break, to allow fall-through in the common case.
		 */
		case State::AttrNsQuot:

			while(buff != end && *buff != '"' && *buff != '\'')
				buff++;

			if(buff == end)
				return true;

			if(!writeEntry('='))
				return false;

			state = State::AttrNsUrl;
			delimiter = *buff;
			buff++;


		/*
		 * Consumes whitespace and equals sign before the url value.
		 * There is no break, to allow fall-through in the common case.
		 */
		case State::AttrNsUrl:
			start = buff;

			while(buff != end && *buff != delimiter)
				buff++;

			if(!writeEntry(start, buff - start))
				return false;

			if(buff == end)
				return true;

			buff++;

			terminateEntry();
			state = State::Attrs;
			break;

		/*
		 * Processes the name of the attribute found, actually passes
		 * the data down until sees some whitespace or an equal sign.
		 * There is no break, to allow fall-through in the common case.
		 */
		case State::AttrName:
			start = buff;

			while(buff != end && (*buff != ' ' && *buff != '='))
				buff++;

			((Child*)this)->onAttributeName(start, buff-start);

			if(buff == end)
				return true;

			((Child*)this)->onAttributeNameEnd();

			buff++;
			state = State::AttrValQuot;

		/*
		 * Consumes the whitespace and the quotation between the equal
		 * sign and the value of the attribute currently being processed.
		 * There is no break, to allow fall-through in the common case.
		 */
		case State::AttrValQuot:
			while(buff != end && (*buff != '"' && *buff != '\''))
				buff++;

			if(buff == end)
				return true;

			((Child*)this)->onAttributeValueStart();


			state = State::AttrVal;
			delimiter = *buff;
			buff++;
		/*
		 * Processes the value of the attribute found, actually passes
		 * data down to the application, until sees the quotation mark.
		 * There is no break, to allow fall-through in the common case.
		 */
		case State::AttrVal:
			start = buff;

			while(buff != end && *buff != delimiter)
				buff++;

			((Child*)this)->onAttributeValue(start, buff-start);

			if(buff == end)
				return true;

			((Child*)this)->onAttributeValueEnd();

			buff++;
			state = State::Attrs;
			break;

		/*
		 * Common fall-back consumer for all kinds of unimplemented
		 * features, like PIs and DTD stuff, also all the closing tags
		 */
		case State::WaitClose:
			while(buff != end && *buff != '>')
				buff++;

			if(buff == end)
				return true;

			buff++;

			state = State::Content;
			break;
		}
	}

	return true;
}

/*
 * Can be called when the end of input is reached to check the integrity.
 * Returns true if the parsing went OK.
 */
template<class Child, unsigned int stackSize>
inline bool UXml<Child, stackSize>::done()
{
	return !stackIdx;
}


#endif /* UXML_H_ */
