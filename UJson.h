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

#ifndef UJSON_H_
#define UJSON_H_

#include <stddef.h>

#include <IntParser.h>
#include <Keywords.h>

enum class JsonValueType { Null, String, Number, Boolean, Array, Object };

template<class Child, uint16_t maxDepth>
class UJson {
        class BitStack {
                uint16_t idx;
                uint8_t data[(maxDepth + 7) / 8];
            public:
                inline void reset() {
                    idx = (uint16_t) -1u;
                }

                bool push() {
                    if((uint16_t)(idx + 1) >= maxDepth)
                        return false;

                    idx++;
                    return true;
                }

                void pop() {
                    idx--;
                }

                bool isEmpty() {
                	return idx == (uint16_t)-1u;
                }

                bool read() {
                    return (data[idx/8] >> (idx % 8)) & 1;
                }

                void write(bool x) {
                    const uint8_t mask = 1 << (idx % 8);
                    data[idx/8] = (x ? (data[idx/8] | mask) : (data[idx/8] & ~mask));
                }
        };

        BitStack stack;

        enum class State: uint8_t {
            BeforeValue,
            InString,
            InStringQuote,
            InNumber,
            InLiteral,
            AfterValue,
            BeforeObjColon,
            BeforeObjKey,
        } state;

        bool inObjKey;

        enum class Literal: uint8_t {Null, True, False};
        typedef Keywords<Literal, 3> LiteralKeywords;

        static constexpr LiteralKeywords literalKeywords = {
            typename LiteralKeywords::Keyword("null", Literal::Null),
            typename LiteralKeywords::Keyword("true", Literal::True),
            typename LiteralKeywords::Keyword("false", Literal::False),
        };

        union {
                IntParser intParser;
                typename LiteralKeywords::Matcher literalMatcher;
        };

        enum class EntityType {
            Root, Object, Array
        };

    protected:
        /*
         * To be imlemented by the user.
         */

        inline void beforeKey() {}
        inline void onKey(const char *at, size_t length) {}
        inline void onKeyError() {}
        inline void afterKey() {}

        inline void beforeValue(JsonValueType) {}
        inline void onNull() {}
        inline void onBoolean(bool) {}
        inline void onNumber(int32_t value) {}
        inline void onString(const char *at, size_t length) {}
        inline void onValueError() {}
        inline void afterValue(JsonValueType) {}

        inline void onStructureError() {}
        inline void onResourceError() {}

        /*
         * Internal helpers.
         */

        inline void flushLiteral() {
            if(const typename LiteralKeywords::Keyword* result = literalMatcher.match(literalKeywords)) {
                if(result->getValue() == Literal::Null) {
                    ((Child*)this)->beforeValue(JsonValueType::Null);
                    ((Child*)this)->onNull();
                    ((Child*)this)->afterValue(JsonValueType::Null);
                } else {
                    ((Child*)this)->beforeValue(JsonValueType::Boolean);
                    ((Child*)this)->onBoolean(result->getValue() == Literal::True);
                    ((Child*)this)->afterValue(JsonValueType::Boolean);
                }
            } else {
                ((Child*)this)->onValueError();
            }
        }

        inline void flushNumber() {
            ((Child*)this)->onNumber(intParser.getData());
            ((Child*)this)->afterValue(JsonValueType::Number);
        }

        inline void flushObject() {
            if(currentEntity() != EntityType::Object)
                ((Child*)this)->onStructureError();

            leave();
        }

        inline void flushArray() {
            if(currentEntity() != EntityType::Array)
                ((Child*)this)->onStructureError();

            leave();
        }

        inline bool enter(EntityType entity) {
            if(!stack.push())
                return false;

            stack.write(entity == EntityType::Object);
            return true;
        }

        inline EntityType currentEntity()
        {
            if(stack.isEmpty())
                return EntityType::Root;

            return stack.read() ? EntityType::Object : EntityType::Array;
        }

        inline void leave()
        {
            state = State::AfterValue;

        	if(currentEntity() == EntityType::Object)
        		((Child*)this)->afterValue(JsonValueType::Object);
        	else if(currentEntity() == EntityType::Array)
                ((Child*)this)->afterValue(JsonValueType::Array);
        	else
        		return;

            stack.pop();
        }

        static inline bool isWs(char c) {
            return c == ' ' || c == '\t' || c == '\r' || c == '\n';
        }

        static inline bool isDigit(char c) {
            return '0' <= c && c <= '9';
        }

    public:
        inline void reset() {
            stack.reset();
            state = State::BeforeValue;
            inObjKey = false;
        }

        inline bool parse(const char* buff, uint32_t length) {
            auto self = static_cast<Child*>(this);
            const char* const end = buff + length;
            const char* start;

            while(buff != end) {
                switch(state) {
                    case State::BeforeValue:
                        while(buff != end) {
                            if(isWs(*buff)) {
                                buff++;
                                continue;
                            }

                            if(*buff == '-' || isDigit(*buff)) {
                                state = State::InNumber;
                                ((Child*)this)->beforeValue(JsonValueType::Number);
                                intParser.reset();
                            } else if(*buff == '\"'){
                                state = State::InString;
                                ((Child*)this)->beforeValue(JsonValueType::String);
                                buff++;
                            } else if(*buff == '['){
                                state = State::BeforeValue;

                                if(!enter(EntityType::Array))
                                    ((Child*)this)->onResourceError();
                                else
                                	((Child*)this)->beforeValue(JsonValueType::Array);

                                buff++;
                            } else if(*buff == ']') {
								flushArray();
								buff++;
                            } else if(*buff == '{'){
                                state = State::BeforeObjKey;

                                if(!enter(EntityType::Object))
                                    ((Child*)this)->onResourceError();
                                else
                                	((Child*)this)->beforeValue(JsonValueType::Object);

                                buff++;
                            } else {
                                literalMatcher.reset();
                                state = State::InLiteral;
                            }

                            break;
                        }

                        break;
                    case State::InString:
                        start = buff;
                        while(buff != end) {
                            if(*buff == '\"') {
                                state = (inObjKey) ? State::BeforeObjColon : State::AfterValue;
                            } else if(*buff == '\\')
                                state = State::InStringQuote;

                            if(state != State::InString)
                                break;

                            buff++;
                        }

                        (self->*(inObjKey ? &Child::onKey : &Child::onString))(start, buff - start);

                        if(state == State::AfterValue || state == State::BeforeObjColon) {
                            buff++;


                            if(inObjKey) {
                                ((Child*)this)->afterKey();
                                inObjKey = false;
                            } else
                                ((Child*)this)->afterValue(JsonValueType::String);
                        }else if(state == State::InStringQuote)
                        	buff++;

                        break;
                    case State::InStringQuote:
                        switch(*buff) {
                            static constexpr const char backSlash = '\\';
                            static constexpr const char slash = '/';
                            static constexpr const char backSpace = '\b';
                            static constexpr const char formFeed = '\f';
                            static constexpr const char newLine = '\n';
                            static constexpr const char carriageReturn = '\r';
                            static constexpr const char tab = '\t';

                            case '\\': (self->*(inObjKey ? &Child::onKey : &Child::onString))(&backSlash, 1); break;
                            case '/': (self->*(inObjKey ? &Child::onKey : &Child::onString))(&slash, 1); break;
                            case 'b': (self->*(inObjKey ? &Child::onKey : &Child::onString))(&backSpace, 1); break;
                            case 'f': (self->*(inObjKey ? &Child::onKey : &Child::onString))(&formFeed, 1); break;
                            case 'n': (self->*(inObjKey ? &Child::onKey : &Child::onString))(&newLine, 1); break;
                            case 'r': (self->*(inObjKey ? &Child::onKey : &Child::onString))(&carriageReturn, 1); break;
                            case 't': (self->*(inObjKey ? &Child::onKey : &Child::onString))(&tab, 1); break;
                            default: (self->*(inObjKey ? &Child::onKeyError : &Child::onValueError))(); break;
                        }

                        state = State::InString;
                        buff++;
                        break;
                    case State::InNumber:
                        start = buff;

                        while(buff != end && (*buff == '-' || isDigit(*buff)))
                            buff++;

                        intParser.parseInt(start, buff - start);

                        if(buff != end) {
                        	flushNumber();
                            state = State::AfterValue;
                        }

                        break;
                    case State::InLiteral:
                        start = buff;

                        while(buff != end && !isWs(*buff) && *buff != ',' && *buff != ']' && *buff != '}')
                            buff++;

                        if(!literalMatcher.progress(literalKeywords, start, buff-start)) {
                            ((Child*)this)->onValueError();
                            state = State::AfterValue;
                        } else {
                            if(buff != end) {
                            	flushLiteral();

                                state = State::AfterValue;
                            }
                        }

                        break;
                    case State::AfterValue:
                        while(buff != end) {
                            if(!isWs(*buff)) {
                                if(*buff == ','){
                                	if(currentEntity() == EntityType::Object)
                                		state = State::BeforeObjKey;
                                	else {
                                		if(currentEntity() == EntityType::Root)
                                			((Child*)this)->onStructureError();

                                		state = State::BeforeValue;
                                	}
                                } else if(*buff == ']'){
                                	flushArray();
                                } else if(*buff == '}'){
                                	flushObject();
                                }

                                buff++;
                                break;
                            }

                            buff++;
                        }

                        break;
                    case State::BeforeObjKey:
                        while(buff != end) {
                            if(!isWs(*buff)) {
                                if(*buff == '\"'){
                                    inObjKey = true;
                                    state = State::InString;
                                    ((Child*)this)->beforeKey();
                                    buff++;
                                    break;
                                } else if(*buff == '}') {
                                	flushObject();
                                	buff++;
                                	break;
                                } else
                                    ((Child*)this)->onKeyError();
                            }

                            buff++;
                        }
                        break;
                    case State::BeforeObjColon:
                        while(buff != end) {
                            if(!isWs(*buff)) {
                                if(*buff == ':'){
                                    state = State::BeforeValue;
                                    buff++;
                                    break;
                                } else if(*buff == '}') {
                                	flushObject();
                                	buff++;
                                	break;
                                } else
                                    ((Child*)this)->onKeyError();
                            }

                            buff++;
                        }

                        break;
                }
            }

            return true;
        }

        inline bool done() {
            switch(state) {
                case State::InNumber:
                	flushNumber();
                	break;
                case State::InLiteral:
                	flushLiteral();
                	break;
                case State::InStringQuote:
                case State::InString:
                	((Child*)this)->afterValue(JsonValueType::String);
                	((Child*)this)->onStructureError();
                	break;
                case State::BeforeValue:
                case State::BeforeObjColon:
                case State::BeforeObjKey:
                	((Child*)this)->onStructureError();
                	break;

                case State::AfterValue:
                	break;
            }

            return true;
        }
};

template<class Child, uint16_t maxDepth>
constexpr typename UJson<Child, maxDepth>::LiteralKeywords UJson<Child, maxDepth>::literalKeywords;

#endif /* UJSON_H_ */
