/*
 * PathParser.h
 *
 *  Created on: 2017.02.06.
 *      Author: tooma
 */

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
