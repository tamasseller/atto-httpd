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

/*
 * Non-buffered multiple string matcher object.
 *
 * Features:
 *
 *  - Size of internal state is independent of number or length of keywords.
 *  - Supports processing of arbitrarily fragmented data.
 *  - The immutable problem description (string key, arbitrary value pairs)
 *    are stored in a separate object, to allow for storing it in different,
 *    read-only memory regions (ie. program flash on MCUs).
 */

/**
 * Argument independent mutable keyword matcher state.
 */
struct KeywordMatcherState {
	/// Index of the currently considered mapping (or -1 if there is none).
	uint16_t idx;

	/// Offset into the key of the currently considered mapping.
	uint16_t offset;
};

/**
 * Constant keyword set descriptor.
 *
 * Description for matching a set of arbitrary strings and
 * mapping them to statically associated values.
 *
 * The _T_ parameter is the type the keywords are associated
 * with and _N_ is the number of the these mappings.
 */
template<class T, unsigned int N>
class Keywords {
public:

	/**
	 * Keyword entry.
	 *
	 * A pair of the keyword string. Supports forwarding
	 * multiple arguments to the value types constructor.
	 */
	class Keyword {
		friend Keywords;
		const char* key;
		T value;
	public:
		/// Argument forwarding constructor.
		template<class... V>
		explicit constexpr Keyword(const char* key, const V&... args): key(key), value(args...) {}

		/// Read only key accessor.
		inline const char* getKey() const {
			return key;
		}

		/// Read only value accessor.
		inline const T getValue() const {
			return value;
		}
	};
private:
	/// Storage of the mappings.
	Keyword words[N];

public:
	template<class... V>
	constexpr Keywords(V... args): words{args...} {}

	/// STL compatible accessor for iterating over the stored mappings.
	const Keyword* begin() {
		return words;
	}

	/// STL compatible accessor for iterating over the stored mappings.
	const Keyword* end() {
		return words + N;
	}

	/**
	 * Matcher logic, without attached state.
	 */
	struct DetachedMatcher {
		/// Initialize internal state.
		static inline void reset(KeywordMatcherState* state) {
			state->idx = 0;
			state->offset = 0;
		}

		/// Process a block of input data.
		static inline bool progress(KeywordMatcherState* state, const Keywords& kw, const char* str, uint32_t len)
		{
			// Return if input is already known to be non-matching.
			if(state->idx == 0xffff)
				return false;

			while(len--) {
				// If the current candidate fails at the next character.
				if(*str != kw.words[state->idx].key[state->offset]) {
					// Go over every other mapping.
					uint16_t altidx = state->idx;
					do {
						altidx = (altidx != N - 1) ? (altidx + 1) : 0;

						/*
						 * Exit the loop if the alternative has the same
						 * prefix as the failed candidate and also has
						 * the right next character.
						 */
						if(state->offset < strlen(kw.words[altidx].key) &&
								*str == kw.words[altidx].key[state->offset] &&
								strncmp(kw.words[state->idx].key, kw.words[altidx].key, state->offset) == 0)
							break;
					} while(altidx != state->idx);

					// The loop returned to the starting point, giving up.
					if(altidx == state->idx) {
						state->idx = 0xffff;
						return false;
					}

					/*
					 * If the loop exited before reaching starting point
					 * the alternative is satisfactory.
					 */
					state->idx = altidx;
				}

				/*
				 * If this point is reached either the candidate is
				 * still OK, or a better alternative is found.
				 */
				state->offset++;
				str++;
			}

			return true;
		}

		/// Process a zero-terminated string.
		static inline bool progress(KeywordMatcherState* state, const Keywords& kw, const char* str)
		{
			return progress(state, kw, str, strlen(str));
		}

		/**
		 * Do final checking on the current state.
		 *
		 * Checks if the current (possibly partial) match is complete.
		 *
		 * @return Returns the fully matched result, or null if there is none.
		 */
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

	/// Matcher object with the embedded state.
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
