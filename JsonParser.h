/*
 * JsonParser.h
 *
 *      Author: tamas.seller
 */

#ifndef JSONPARSER_H_
#define JSONPARSER_H_

#include "UJson.h"
#include "Keywords.h"

class EntityFilter {
	template<uint16_t> friend class JsonParser;

	EntityFilter* parent;
	uint32_t depth;

protected:
	struct Parser {
		virtual void enter(EntityFilter*) = 0;
	};

private:
	virtual void beforeKey() {}
	virtual void onKey(const char *at, size_t length) {}
	virtual void afterKey() {}
	virtual void beforeValue(Parser*, JsonValueType) {}
	virtual void onNull() {}
	virtual void onBoolean(bool) {}
	virtual void onNumber(int32_t value) {}
	virtual void onString(const char *at, size_t length) {}
	virtual void afterValue(JsonValueType) {}

public:
	virtual void reset(EntityFilter* parent) {this->parent = parent;}

	inline virtual ~EntityFilter() {}
};

class FilterEntry {
		template<size_t> friend class ArrayFilter;
		template<size_t> friend class ObjectFilter;

		union {
			uint32_t idx;
			const char* name;
		};

		EntityFilter* child;

	public:
		FilterEntry (uint32_t idx, EntityFilter* child): idx(idx), child(child) {}
		FilterEntry (const char* name, EntityFilter* child): name(name), child(child) {}

};

template<size_t n>
class ArrayFilter: public EntityFilter {
	template<uint16_t> friend class JsonParser;

public:
	FilterEntry entries[n];
	uint32_t idx;

	template<class... T>
	inline ArrayFilter(T... t): entries{t...} {}
	inline virtual ~ArrayFilter() {}

private:
	inline virtual void reset(EntityFilter* parent) {
		EntityFilter::reset(parent);
		idx = 0;

		for(auto e: entries)
			e.child->reset(this);
	}

	inline virtual void beforeValue(Parser* parser, JsonValueType) {
		for(auto e: entries) {
			if(e.idx == idx) {
				parser->enter(e.child);
				break;
			}
		}

		idx++;
	}
};

class NumberExtractor: public EntityFilter {
	int &result;

	inline virtual void onNumber(int32_t value) {
		result = value;
	}
public:
	inline NumberExtractor(int &result): result(result) {}
};

template<template<size_t> class T, class... A>
T<sizeof...(A)> assemble(A... a) {
	return T<sizeof...(A)>(a...);
}

template<uint16_t maxDepth>
class JsonParser: public UJson<JsonParser<maxDepth>, maxDepth>, public EntityFilter::Parser
{
        typedef UJson<JsonParser, maxDepth> Parent;
        friend Parent;

        EntityFilter *filter;
        bool error;
        uint32_t depth;

        inline void beforeKey() {
            if(!error && filter)
                filter->beforeKey();
        }

        inline void onKey(const char *at, size_t length) {
            if(!error && filter)
                filter->onKey(at, length);
        }

        inline void afterKey() {
            if(!error && filter)
                filter->afterKey();
        }

        inline void beforeValue(JsonValueType type) {
        	if(depth++) {
				if(!error && filter)
					filter->beforeValue(this, type);

				filter->depth++;
        	}
        }

        inline void onNull() {
            if(!error && filter)
                filter->onNull();
        }

        inline void onBoolean(bool x) {
            if(!error && filter)
                filter->onBoolean(x);
        }

        inline void onNumber(int32_t value) {
            if(!error && filter)
                filter->onNumber(value);
        }

        inline void onString(const char *at, size_t length) {
            if(!error && filter)
                filter->onString(at, length);
        }

        inline void afterValue(JsonValueType type)
        {
        	if(depth--) {
				if(!--filter->depth)
					filter = filter->parent;

				if(!error && filter)
					filter->afterValue(type);
        	}
        }

        virtual void enter(EntityFilter *filter) {
        	filter->parent = this->filter;
        	filter->depth = 0;
        	this->filter = filter;
        }

        inline void onKeyError() {error = true;}
        inline void onValueError() {error = true;}
        inline void onStructureError() {error = true;}
        inline void onResourceError() {error = true;}

    public:
        void reset(EntityFilter *filter) {
            Parent::reset();
            error = false;
            depth = 0;
            this->filter = filter;
            this->filter->reset(nullptr);
        }

        inline virtual ~JsonParser(){}
};

#endif /* JSONPARSER_H_ */
