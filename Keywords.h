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
#ifndef KEYWORDS_H_
#define KEYWORDS_H_

#include <stdint.h>
#include <string.h>

struct KeywordMatcherState {
	uint16_t idx, offset;
};

template<class T, unsigned int N>
class Keywords {
public:
	class Keyword {
		friend Keywords;
		const char* key;
		T value;
	public:
		template<class... V>
		explicit constexpr Keyword(const char* key, const V&... args): key(key), value(args...) {}

		inline const char* getKey() const {
			return key;
		}

		inline const T getValue() const {
			return value;
		}
	};
private:
	Keyword words[N];

public:
	template<class... V>
	constexpr Keywords(V... args): words{args...} {}

	struct DetachedMatcher {
		static inline void reset(KeywordMatcherState* state) {
			state->idx = 0;
			state->offset = 0;
		}

		static inline bool progress(KeywordMatcherState* state, const Keywords& kw, const char* str, uint32_t len)
		{
			if(state->idx == 0xffff)
				return false;

			while(len--) {
				if(*str != kw.words[state->idx].key[state->offset]) {
					uint16_t altidx = state->idx;
					do {
						altidx = (altidx != N - 1) ? (altidx + 1) : 0;
						if(state->offset < strlen(kw.words[altidx].key) &&
								*str == kw.words[altidx].key[state->offset] &&
								strncmp(kw.words[state->idx].key, kw.words[altidx].key, state->offset) == 0)
							break;
					} while(altidx != state->idx);

					if(altidx == state->idx) {
						state->idx = 0xffff;
						return false;
					}

					state->idx = altidx;
				}

				state->offset++;
				str++;
			}
			return true;
		}

		static inline bool progress(KeywordMatcherState* state, const Keywords& kw, const char* str)
		{
			return progress(state, kw, str, strlen(str));
		}

		static inline const Keyword* match(KeywordMatcherState* state, const Keywords& kw)
		{
			if(state->idx != 0xffff) {
				if(state->offset == strlen(kw.words[state->idx].key))
					return &kw.words[state->idx];

				for(int sidx = N-1; sidx >= 0; sidx--) {
					if(	(sidx != state->idx) &&
						(state->offset == strlen(kw.words[sidx].key)) &&
						(strncmp(kw.words[state->idx].key, kw.words[sidx].key, state->offset) == 0))
						return &kw.words[sidx];
				}
			}

			return 0;
		}
	};

	struct Matcher: private DetachedMatcher, private KeywordMatcherState {
		inline void reset() {
			DetachedMatcher::reset(this);
		}

		inline bool progress(const Keywords& kw, const char* str, uint32_t len) {
			return DetachedMatcher::progress(this, kw, str, len);
		}

		inline bool progress(const Keywords& kw, const char* str) {
			return DetachedMatcher::progress(this, kw, str);
		}

		inline const Keyword* match(const Keywords& kw) {
			return DetachedMatcher::match(this, kw);
		}
	};
};

#endif /* KEYWORDS_H_ */
