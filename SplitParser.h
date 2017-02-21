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
#ifndef SPLITPARSER_H_
#define SPLITPARSER_H_

template <class Child>
class Splitter {
        bool inStr;
        bool hasData;

    public:
        void progressWithSplitting(const char* buff, unsigned int length)
        {
            if(!length)
                return;

            const char* start = buff;
            while(length--) {
                const char c = *buff++;

                if((c == ' ' || c == ',') && !inStr) {
                    if(hasData) {
                        hasData = false;
                        ((Child*)this)->parseField(start, buff-start-1);
                        ((Child*)this)->fieldDone();
                    }
                    start = buff;
                } else {
                    hasData = true;
                    if(c == '"') {
                        inStr = !inStr;
                    }
                }
            }

            if(hasData) {
                ((Child*)this)->parseField(start, buff-start);
            }
        }
        void splittingDone() {
            ((Child*)this)->fieldDone();
        }
        void reset() {
            inStr = false;
            hasData = false;
        }
};

#endif /* SPLITPARSER_H_ */
