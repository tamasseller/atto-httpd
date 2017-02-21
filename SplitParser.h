/*
 * SplitParser.h
 *
 *  Created on: 2017.02.06.
 *      Author: tooma
 */

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
