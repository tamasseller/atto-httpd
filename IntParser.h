/*
 * IntParser.h
 *
 *      Author: tamas.seller
 */

#ifndef INTPARSER_H_
#define INTPARSER_H_

#include <stdint.h>

class IntParser {
    int32_t data;
    enum class SignState: uint8_t {NYET, PLUS, MINUS, DONE} sign;
public:

    inline void reset() {
        data = 0;
        sign = SignState::NYET;
    }

    inline bool parseInt(const char *at, uint32_t length)
    {
        for(; length--; at++) {
            if(*at == '-') {
                if(sign != SignState::NYET)
                    return false;

                sign = SignState::MINUS;
            } else if('0' <= *at && *at <= '9') {
                int add = (data < 0) ? -(*at - '0') : (*at - '0');
                data = data * 10 + add;

                if(sign == SignState::NYET)
                    sign = SignState::PLUS;

                if(data && sign != SignState::DONE) {
                    if(sign == SignState::MINUS)
                        data *= -1;

                    sign = SignState::DONE;
                }
            } else
                return false;
        }

        return true;
    }


    inline int32_t getData()
    {
        return data;
    }
};




#endif /* INTPARSER_H_ */
