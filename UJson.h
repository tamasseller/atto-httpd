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
                    if((uint16_t)(idx + 1) > maxDepth)
                        return false;

                    idx++;
                    return true;
                }

                bool pop() {
                    if(idx == -1u)
                        return false;

                    idx--;
                    return true;
                }

                bool isEmpty() {
                    return idx == -1u;
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

        enum class EntityType {
            Root, Object, Array
        };

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

        inline bool leave()
        {
            return stack.pop();
        }

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

        static inline bool isWs(char c) {
            return c == ' ' || c == '\t' || c == '\r' || c == '\n';
        }

        static inline bool isDigit(char c) {
            return '0' <= c && c <= '9';
        }
    protected:
        /*
         * To be imlemented by the user.
         */

        enum class ValueType { Null, String, Number, Boolean, Array, Object };

        inline void beforeKey() {}
        inline void onKey(const char *at, size_t length) {}
        inline void onKeyError() {}
        inline void afterKey() {}

        inline void beforeValue(ValueType) {}
        inline void onNull() {}
        inline void onBoolean(bool) {}
        inline void onNumber(int32_t value) {}
        inline void onString(const char *at, size_t length) {}
        inline void onValueError() {}
        inline void afterValue(ValueType) {}

        inline void onStructureError() {}
        inline void onResourceError() {}
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
                                ((Child*)this)->beforeValue(ValueType::Number);
                            } else if(*buff == '\"'){
                                state = State::InString;
                                ((Child*)this)->beforeValue(ValueType::String);
                                buff++;
                            } else if(*buff == '['){
                                state = State::BeforeValue;

                                if(!enter(EntityType::Array))
                                    ((Child*)this)->onResourceError();

                                ((Child*)this)->beforeValue(ValueType::Array);
                                buff++;
                            } else if(*buff == '{'){
                                state = State::BeforeObjKey;

                                if(!enter(EntityType::Object))
                                    ((Child*)this)->onResourceError();

                                ((Child*)this)->beforeValue(ValueType::Object);
                                buff++;
                            } else
                                state = State::InLiteral;

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
                                ((Child*)this)->afterValue(ValueType::String);
                        }

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
                            default: ((Child*)this)->onValueError(); break;
                        }

                        state = State::InString;
                        buff++;
                        break;
                    case State::InNumber:
                        break;
                    case State::InLiteral:
                        break;
                    case State::AfterValue:
                        while(buff != end) {
                            if(!isWs(*buff)) {
                                if(*buff == ','){
                                    if(currentEntity() == EntityType::Root)
                                        ((Child*)this)->onStructureError();
                                    else if(currentEntity() == EntityType::Object)
                                        state = State::BeforeObjKey;
                                    else
                                        state = State::BeforeValue;

                                } else if(*buff == ']'){
                                    if(currentEntity() != EntityType::Array)
                                        ((Child*)this)->onStructureError();
                                    else
                                        ((Child*)this)->afterValue(ValueType::Array);

                                    leave();
                                    state = State::AfterValue;
                                } else if(*buff == '}'){
                                    if(currentEntity() != EntityType::Object)
                                        ((Child*)this)->onStructureError();
                                    else
                                        ((Child*)this)->afterValue(ValueType::Object);

                                    leave();
                                    state = State::AfterValue;
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
            return true;
        }
};

#endif /* UJSON_H_ */
