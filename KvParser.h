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
