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
#ifndef HEXPARSER_H_
#define HEXPARSER_H_

#include <stdint.h>

template<unsigned int size>
class HexParser {
	uint32_t idx;
	char storage[size];
public:
	inline void clear();
	inline void parseHex(const char *at, uint32_t length);
	inline const char *data();
	inline bool isDone();
};

template<unsigned int size>
inline void HexParser<size>::clear()
{
	idx = 0;
}

template<unsigned int size>
inline void HexParser<size>::parseHex(const char *at, uint32_t length)
{
	if(idx == -1u)
		return;

	if(length + idx > 2 * size)
		idx = -1u;
	else {
		while(length--) {
			char digit;

			if('0' <= *at && *at <= '9')
				digit = *at - '0';
			else if('a' <= *at && *at <= 'f')
				digit = *at - 'a' + 10;
			else if('A' <= *at && *at <= 'F')
				digit = *at - 'A' + 10;
			else {
				idx = -1u;
				return;
			}

			if(idx % 2)
				storage[idx / 2] |= digit;
			else
				storage[idx / 2] = digit << 4;

			idx++;
			at++;
		}
	}
}

template<unsigned int size>
inline const char *HexParser<size>::data() {
	return storage;
}

template<unsigned int size>
inline bool HexParser<size>::isDone() {
	return idx == 2 * size;
}

#endif /* HEXPARSER_H_ */
