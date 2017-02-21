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
#ifndef PATHPARSER_H_
#define PATHPARSER_H_

template<class Child>
class PathParser {
    bool hasData;

public:
    void parsePath(const char* buff, unsigned int length)
    {
        if(!length)
            return;

        const char* start = buff;
        bool hadData = hasData;
        while(length--) {
            const char c = *buff++;

            if(c == '/') {
                if(hasData) {
                	if(!hadData)
                		((Child*)this)->beforeElement();

                	hadData = hasData = false;
                    ((Child*)this)->parseElement(start, buff-start-1);
                    ((Child*)this)->elementDone();
                }
                start = buff;
            } else {
                hasData = true;
            }
        }

        if(hasData) {
        	if(!hadData)
        		((Child*)this)->beforeElement();

            ((Child*)this)->parseElement(start, buff-start);
        }
    }
    void done() {
    	if(hasData)
    		((Child*)this)->elementDone();
    }

    void reset() {
    	hasData = false;
    }
};



#endif /* PATHPARSER_H_ */
