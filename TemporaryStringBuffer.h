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
#ifndef TEMPORARYSTRINGBUFFER_H_
#define TEMPORARYSTRINGBUFFER_H_

#include <stdint.h>
#include <string.h>

template<unsigned int size>
class TemporaryStringBuffer {
	uint32_t idx;
	char storage[size];
public:
	inline void clear();
	inline bool save(const char *at, uint32_t length);
	inline void terminate();
	inline const char *data();
	inline uint32_t length();
};

template<unsigned int size>
inline void TemporaryStringBuffer<size>::clear()
{
	idx = 0;
}

template<unsigned int size>
inline bool TemporaryStringBuffer<size>::save(const char *at, uint32_t length)
{
	bool ret = true;

	if(length + idx > size - 1) {
		ret = false;
		length = size - 1 - idx;
	}

	memcpy(storage + idx, at, length);
	idx += length;

	return ret;
}

template<unsigned int size>
inline void TemporaryStringBuffer<size>::terminate()
{
	storage[idx] = '\0';
}


template<unsigned int size>
inline const char *TemporaryStringBuffer<size>::data() {
	return storage;
}

template<unsigned int size>
inline uint32_t TemporaryStringBuffer<size>::length() {
	return idx;
}


#endif /* TEMPORARYSTRINGBUFFER_H_ */
