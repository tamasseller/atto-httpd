/*
 * Keywords.h
 *
 *  Created on: 2017.01.21.
 *      Author: tooma
 */

#ifndef KEYWORDS_H_
#define KEYWORDS_H_

#include <stdint.h>
#include <string.h>

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

	class Matcher {
		uint16_t idx, offset;
	public:
		void reset() {
			idx = 0;
			offset = 0;
		}

		bool progress(const Keywords& kw, const char* str, uint32_t len)
		{
			if(idx == 0xffff)
				return false;

			while(len--) {
				if(*str != kw.words[idx].key[offset]) {
					uint16_t altidx = idx;
					do {
						altidx = (altidx != N - 1) ? (altidx + 1) : 0;
						if(offset < strlen(kw.words[altidx].key) &&
								*str == kw.words[altidx].key[offset] &&
								strncmp(kw.words[idx].key, kw.words[altidx].key, offset) == 0)
							break;
					} while(altidx != idx);

					if(altidx == idx) {
						idx = 0xffff;
						return false;
					}

					idx = altidx;
				}

				offset++;
				str++;
			}
			return true;
		}

		bool progress(const Keywords& kw, const char* str)
		{
			return progress(kw, str, strlen(str));
		}

		const Keyword* match(const Keywords& kw)
		{
			if(idx != 0xffff) {
				if(offset == strlen(kw.words[idx].key))
					return &kw.words[idx];

				for(int sidx = N-1; sidx >= 0; sidx--) {
					if(	(sidx != idx) &&
						(offset == strlen(kw.words[sidx].key)) &&
						(strncmp(kw.words[idx].key, kw.words[sidx].key, offset) == 0))
						return &kw.words[sidx];
				}
			}

			return 0;
		}
	};
};

#endif /* KEYWORDS_H_ */
