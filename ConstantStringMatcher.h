/*
 * ConstantStringMatcher.h
 *
 *  Created on: 2017.02.09.
 *      Author: tooma
 */

#ifndef CONSTANTSTRINGMATCHER_H_
#define CONSTANTSTRINGMATCHER_H_

#include <stdint.h>
#include <string.h>

class ConstantStringMatcher{
	uint32_t idx;
public:
	void progressWithMatching(const char* str, const char* buff, uint32_t length)
	{
		if(idx == -1u)
			return;

		if(idx + length > strlen(str))
			idx = -1u;
		else {
			if(strncmp(str + idx, buff, length) != 0)
				idx = -1u;
			else
				idx += length;
		}
	}

	void reset()
	{
		idx = 0;
	}

	bool matches(const char* str)
	{
		return idx == strlen(str);
	}

};

#endif /* CONSTANTSTRINGMATCHER_H_ */
