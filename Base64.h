/*
 * Base64.h
 *
 *  Created on: 2017.10.04.
 *      Author: tooma
 */

#ifndef BASE64_H_
#define BASE64_H_

#include "meta/Sequence.h"

namespace detail {
	template<const char (&forward)[64], class> struct Reversor;

	template<const char (&forward)[64], int... i>
	struct Reversor<forward, pet::Sequence<i...>> {
		template<int k>
		static constexpr char find(char needle, const pet::Sequence<k> &) {
			return (needle == forward[k] ? k : -1);
		}

		template<int k, int... j>
		static constexpr char find(char needle, const pet::Sequence<k, j...> &) {
			return find(needle, pet::Sequence<k>()) & find(needle, pet::Sequence<j...>());
		}

		static constexpr const char value[] = {find(i, pet::sequence<0, 64>())...};
	};

	template<const char (&forward)[64], int... i>
	constexpr const char Reversor<forward, pet::Sequence<i...>>::value[];

	struct Alphabet {
		static constexpr const char forwardLut[64] = {
				'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
				'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
				'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
				'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'};

		static constexpr const char (&reverseLut)[256] = Reversor<forwardLut, pet::sequence<0, 256>>::value;
	};

	constexpr const char Alphabet::forwardLut[];
}

struct Base64 {
	class State {
		uint8_t data;

	public:
		uint8_t getIdx() {
			return data & 3;
		}

		void setIdx(uint8_t x) {
			data = (data & ~3) | x;
		}

		uint8_t getLeftover() {
			return data >> 2;
		}

		void setLeftover(uint8_t x) {
			data = (data & 3) | (x << 2);
		}

		void reset() {
			data = 0;
		}
	} ;

public:
	template<class Child>
	struct Parser {
			State state;

	protected:
		/*
		 * User interface
		 */
		inline void byteDecoded(char ) {}

	public:
		void reset() {
			state.reset();
		}

		inline bool parse(const char* buff, uint32_t length) {
			auto self = static_cast<Child*>(this);

			for(const char* const end = buff + length; buff != end;) {
				uint8_t value = detail::Alphabet::reverseLut[(unsigned char)*buff];

				if(value == (uint8_t)-1) {
					if(*buff != '=')
						return false;

					state.setIdx(0);
					buff++;
					continue;
				}

				switch(state.getIdx()) {
				case 0:
				    if(end-buff >= 4) {
				        uint8_t value2 = detail::Alphabet::reverseLut[(unsigned char)buff[1]];
				        uint8_t value3 = detail::Alphabet::reverseLut[(unsigned char)buff[2]];
				        uint8_t value4 = detail::Alphabet::reverseLut[(unsigned char)buff[3]];

				        if(value2 == -1)
				            return false;

                        self->byteDecoded(value << 2 | value2 >> 4);

		                if(value3 == (uint8_t)-1)
		                    return buff[2] == '=';

                        self->byteDecoded(value2 << 4 | value3 >> 2);

                        if(value4 == (uint8_t)-1)
                            return buff[3] == '=';

                        self->byteDecoded(value3 << 6 | value4);

				        buff += 4;
				        continue;
				    }
					state.setLeftover(value);
					break;
				case 1:
					self->byteDecoded(state.getLeftover() << 2 | value >> 4);
					state.setLeftover(value);
					break;
				case 2:
					self->byteDecoded(state.getLeftover() << 4 | value >> 2);
					state.setLeftover(value);
					break;
				case 3:
					self->byteDecoded(state.getLeftover() << 6 | value);
					break;
				}

				state.setIdx((state.getIdx() + 1) & 3);
				buff++;
			}

			return true;
		}

		bool done() {
			return state.getIdx() == 0;
		}
	};

	template<class Child>
	struct Formater {
			State state;

	protected:
		/*
		 * User interface
		 */
		inline void byteEncoded(char ) {}

	public:
		void reset() {
			state.reset();
		}

		inline void format(const char* buff, uint32_t length) {
			auto self = static_cast<Child*>(this);

			for(const char* const end = buff + length; buff != end;) {
				uint8_t value = (unsigned char)*buff;

				switch(state.getIdx()) {
				case 0:
                    if(end-buff >= 3) {
                       uint8_t value2 = buff[1];
                       uint8_t value3 = buff[2];

                       self->byteEncoded(detail::Alphabet::forwardLut[value >> 2]);
                       self->byteEncoded(detail::Alphabet::forwardLut[value2 >> 4 | (value & 0x3) << 4]);
                       self->byteEncoded(detail::Alphabet::forwardLut[value3 >> 6 | (value2 & 0xf )<< 2]);
                       self->byteEncoded(detail::Alphabet::forwardLut[value3 & 0x3f]);
                       buff += 3;
                       continue;
                    }
					self->byteEncoded(detail::Alphabet::forwardLut[value >> 2]);
					state.setLeftover(value & 0x03);
					state.setIdx(1);
					break;
				case 1:
					self->byteEncoded(detail::Alphabet::forwardLut[value >> 4 | state.getLeftover() << 4]);
					state.setLeftover(value & 0x0f);
					state.setIdx(2);
					break;
				case 2:
					self->byteEncoded(detail::Alphabet::forwardLut[value >> 6 | state.getLeftover() << 2]);
					self->byteEncoded(detail::Alphabet::forwardLut[value & 0x3f]);
					state.setIdx(0);
					break;
				}

				buff++;
			}
		}

		void done() {
			auto self = static_cast<Child*>(this);

			switch(state.getIdx()) {
			case 1:
				self->byteEncoded(detail::Alphabet::forwardLut[state.getLeftover() << 4]);
				self->byteEncoded('=');
				self->byteEncoded('=');
				break;
			case 2:
				self->byteEncoded(detail::Alphabet::forwardLut[state.getLeftover() << 2]);
				self->byteEncoded('=');
				break;
			default:
				break;
			}
		}
	};

};


#endif /* BASE64_H_ */
