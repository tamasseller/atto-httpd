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
	/// Mapping inversion helper template declaration.
	template<const char (&forward)[64], class> struct Reversor;

	/**
	 * Mapping inversion helper specialization for extracting index sequence.
	 *
	 * The _forward_ parameter is a reference to the forward mapping table.
	 * The parameter pack _i_ is a sequence of numbers, for which the reverse
	 * lookup is done (ie. 0...255).
	 */
	template<const char (&forward)[64], int... i>
	struct Reversor<forward, pet::Sequence<i...>> {
		/**
		 * Check if the element at index _k_ in _forward_ equals _needle_.
		 *
		 * Termination handler and worker, returns _k_ on success, -1 otherwise.
		 */
		template<int k>
		static constexpr char find(char needle, const pet::Sequence<k> &) {
			return (needle == forward[k] ? k : -1);
		}

		/**
		 * Calls the worker
		 *
		 * Returns the index of the element in _forward_ that is equal to
		 * the parameter _needle_ or -1 if not found.
		 *
		 * It does so by recursively calling the terminating _find_ variant
		 * (above) for the values in the parameter _k_ and logical ANDing
		 * their return values together. Because we can expect forward to
		 * contain unique elements and the terminating variant returns -1
		 * (ie all bits one) on mismatch, the result of ANDing together the
		 * returns values is either the index at which the value _needle_
		 * is found or -1.
		 */
		template<int k, int... j>
		static constexpr char find(char needle, const pet::Sequence<k, j...> &) {
			return find(needle, pet::Sequence<k>()) & find(needle, pet::Sequence<j...>());
		}

		/**
		 * Apply the finder to all possible byte values,
		 * and try to match against all 64 forward values.
		 */
		static constexpr const char value[] = {find(i, pet::sequence<0, 64>())...};
	};

	template<const char (&forward)[64], int... i>
	constexpr const char Reversor<forward, pet::Sequence<i...>>::value[];

	/// Forward and reverse alphabet lookup for the encoder and decoder.
	struct Alphabet {
		/// The encoder table.
		static constexpr const char forwardLut[64] = {
				'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
				'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
				'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
				'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'};

		/// The decoder table, meta-programmatically generated from the forward table.
		static constexpr const char (&reverseLut)[256] = Reversor<forwardLut, pet::sequence<0, 256>>::value;
	};

	constexpr const char Alphabet::forwardLut[];
}

/**
 * Non-buffered, base64 encoder and decoder suite.
 *
 *  - Size of internal state is one byte only for both the encoder and decoder.
 *  - Supports processing of arbitrarily fragmented data.
 */
struct Base64 {
	/**
	 * Common state for encoder and decoder.
	 */
	class State {
		uint8_t data;

	public:
		/**
		 * Get the input block index.
		 *
		 * The index of the character next to be processed inside the input
		 * block, which is:
		 *
		 *  - three bytes, for the encoder,
		 *  - and four bytes for the decoder.
		 *
		 *  Thus only two bits are needed to cover any of the two.
		 */
		uint8_t getIdx() {
			return data & 3;
		}

		/**
		 * Set input block index.
		 *
		 * @see getIdx
		 */
		void setIdx(uint8_t x) {
			data = (data & ~3) | x;
		}

		/**
		 * Get the leftover data.
		 *
		 * Get the leftover data from the processing of the previous
		 * input byte, which is the part of the last piece of information
		 * that could not have been output without the next (current)
		 * byte of input, because one of the output bytes are formed
		 * by the combination of the two.
		 */
		uint8_t getLeftover() {
			return data >> 2;
		}

		/**
		 * Set input leftover data.
		 *
		 * @see getLeftover
		 */
		void setLeftover(uint8_t x) {
			data = (data & 3) | (x << 2);
		}

		/// Initialize internal state.
		void reset() {
			data = 0;
		}
	} ;

public:
	/**
	 * Non-buffered base64 decoder.
	 *
	 * The user needs to be in CRTP relation to it, and receives
	 * decoded data via (probably inlined) method call byte-by-byte,
	 */
	template<class Child>
	struct Parser {
		/// Internal state.
		State state;

	protected:
		/*
		 * User interface
		 */
		inline void byteDecoded(char) {}

	public:
		/// Initialize internal state.
		void reset() {
			state.reset();
		}

		/// Process a block of input data.
		inline bool parse(const char* buff, uint32_t length) {
			auto self = static_cast<Child*>(this);

			/*
			 * Iterate over the input byte-by-byte, unless a full group
			 * can be processed at once (handled from inside the loop).
			 */
			for(const char* const end = buff + length; buff != end;) {
				// Look up value, for the input character.
				uint8_t value = detail::Alphabet::reverseLut[(unsigned char)*buff];

				// The value of -1 means that an invalid or padding character was found.
				if(value == (uint8_t)-1) {
					if(*buff != '=')
						return false;

					state.setIdx(0);
					buff++;
					continue;
				}

				switch(state.getIdx()) {
				case 0:
					// Fast forward if full a block of data is in reach and starting a new block.
				    if(end-buff >= 4) {
				    	// Read and look up all four bytes at once.
				        uint8_t value2 = detail::Alphabet::reverseLut[(unsigned char)buff[1]];
				        uint8_t value3 = detail::Alphabet::reverseLut[(unsigned char)buff[2]];
				        uint8_t value4 = detail::Alphabet::reverseLut[(unsigned char)buff[3]];

				        // Only the third and fourth bytes can be padding.
				        if(value2 == -1)
				            return false;

                        self->byteDecoded(value << 2 | value2 >> 4);

                        // The third byte can be padding.
		                if(value3 == (uint8_t)-1)
		                    return buff[2] == '=';

                        self->byteDecoded(value2 << 4 | value3 >> 2);

                        // The fourth byte can be padding.
                        if(value4 == (uint8_t)-1)
                            return buff[3] == '=';

                        self->byteDecoded(value3 << 6 | value4);

                        // Four byte consumed.
				        buff += 4;
				        continue;
				    }

				    // The first byte in a block can not be output alone.
					state.setLeftover(value);
					break;

				case 1:
					// Second input, first output byte.
					self->byteDecoded(state.getLeftover() << 2 | value >> 4);
					state.setLeftover(value);
					break;
				case 2:
					// Third input, second output byte.
					self->byteDecoded(state.getLeftover() << 4 | value >> 2);
					state.setLeftover(value);
					break;
				case 3:
					// Fourth input, third output byte.
					self->byteDecoded(state.getLeftover() << 6 | value);
					break;
				}

				// Move on with the input index if not fast-forwarded.
				state.setIdx((state.getIdx() + 1) & 3);
				buff++;
			}

			return true;
		}

		/// Check correct termination of the input.
		bool done() {
			return state.getIdx() == 0;
		}
	};

	/**
	 * Non-buffered base64 encoder.
	 *
	 * The user needs to be in CRTP relation to it, and receives
	 * encoded data via (probably inlined) method call byte-by-byte,
	 */
	template<class Child>
	struct Formater {
			State state;

	protected:
		/*
		 * User interface
		 */
		inline void byteEncoded(char ) {}

	public:
		/// Initialize internal state.
		void reset() {
			state.reset();
		}

		/// Process a block of input data.
		inline void format(const char* buff, uint32_t length) {
			auto self = static_cast<Child*>(this);

			/*
			 * Iterate over the input byte-by-byte, unless a full group
			 * can be processed at once (handled from inside the loop).
			 */
			for(const char* const end = buff + length; buff != end;) {
				uint8_t value = (unsigned char)*buff;

				switch(state.getIdx()) {
				case 0:
					// Fast forward if full a block of data is in reach and starting a new block.
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

                    // One byte in, one out, some data stored in leftover, move on to next stage.
					self->byteEncoded(detail::Alphabet::forwardLut[value >> 2]);
					state.setLeftover(value & 0x03);
					state.setIdx(1);
					break;
				case 1:
					// Second byte in, second out, stored data replaced in leftover, move on to next stage.
					self->byteEncoded(detail::Alphabet::forwardLut[value >> 4 | state.getLeftover() << 4]);
					state.setLeftover(value & 0x0f);
					state.setIdx(2);
					break;
				case 2:
					// Third byte in, last two of the group out, move back to initial state.
					self->byteEncoded(detail::Alphabet::forwardLut[value >> 6 | state.getLeftover() << 2]);
					self->byteEncoded(detail::Alphabet::forwardLut[value & 0x3f]);
					state.setIdx(0);
					break;
				}

				buff++;
			}
		}

		/// Insert padding for correctly aligned termination (if needed).
		void done() {
			auto self = static_cast<Child*>(this);

			switch(state.getIdx()) {
			case 1:
				// Only one byte in the input group, flush the rest of it, and add padding.
				self->byteEncoded(detail::Alphabet::forwardLut[state.getLeftover() << 4]);
				self->byteEncoded('=');
				self->byteEncoded('=');
				break;
			case 2:
				// Only two bytes in the input group, flush the rest of it, and add padding.
				self->byteEncoded(detail::Alphabet::forwardLut[state.getLeftover() << 2]);
				self->byteEncoded('=');
				break;
			default:
				// Nothing to do for a finished input group of three.
				break;
			}
		}
	};

};


#endif /* BASE64_H_ */
