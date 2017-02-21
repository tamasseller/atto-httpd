/*
 * KvParser.h
 *
 *  Created on: 2017.02.06.
 *      Author: tooma
 */

#ifndef KVPARSER_H_
#define KVPARSER_H_

template <class Child>
class KvParser {
        enum State {
            KEY,
            SEPARATOR,
            VALUE,
            DONE
        } state;

    public:
        void progressWithKv(const char* buff, unsigned int length)
        {
            if(!length)
                return;

            const char* start = buff;
            while(length--) {
                const char c = *buff++;

                switch(state) {
                    case KEY:
                        if(c == ' ' || c == '=') {
                            state = SEPARATOR;
                            ((Child*)this)->parseKey(start, buff-start-1);
                            ((Child*)this)->keyDone();
                            start = buff;
                        }
                        break;
                    case SEPARATOR:
                        if(c != ' ' && c != '"') {
                            state = VALUE;
                        } else
                            start = buff;
                        break;
                    case VALUE:
                        if(c == '"') {
                            ((Child*)this)->parseValue(start, buff-start-1);
                            ((Child*)this)->valueDone();
                            state = DONE;
                        }
                        break;
                    default:;
                }
            }

            if(state == KEY)
                ((Child*)this)->parseKey(start, buff-start);
            else if(state == VALUE)
                ((Child*)this)->parseValue(start, buff-start);
        }

        void kvDone() {
            if(state == KEY)
            	((Child*)this)->keyDone();
            else if(state == VALUE)
            	((Child*)this)->valueDone();
        }

        void reset() {
        	state = KEY;
        }
};

#endif /* KVPARSER_H_ */
