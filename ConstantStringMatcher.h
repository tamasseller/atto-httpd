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
