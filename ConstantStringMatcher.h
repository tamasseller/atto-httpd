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
#ifndef CONSTANTSTRINGMATCHER_H_
#define CONSTANTSTRINGMATCHER_H_

#include <stdint.h>
#include <string.h>

/**
 * Non-buffered matcher for constant strings
 *
 * Features:
 *
 *  - Size of internal state is independent of the length of string to be matched.
 *  - Supports processing of arbitrarily fragmented data.
 *  - The string to be matched is stored separately, to allow for storing it in a
 *    different, read-only memory region (ie. program flash on MCUs).
 */
class ConstantStringMatcher{
	/// Index of the last matched character or -1 on failure.
	uint32_t idx;
public:
	/**
	 * Process a block of data.
	 *
	 * The _str_ parameter is the string to matched, the rest of the
	 * parameters define the block of input data to be processed.
	 */
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

	/// Initialize internal state.
	void reset() {
		idx = 0;
	}

	/// Final check to rule out partial matches. Returns true on match.
	bool matches(const char* str) {
		return idx == strlen(str);
	}
};

#endif /* CONSTANTSTRINGMATCHER_H_ */
