/*
 * JsonParser.h
 *
 *      Author: tamas.seller
 */

#ifndef JSONPARSER_H_
#define JSONPARSER_H_

#include "UJson.h"
#include "Keywords.h"

#include "algorithm/Str.h"

class EntityFilter {
	template<uint16_t> friend class JsonParser;
	template<size_t> friend class ArrayFilter;
	template<size_t> friend class ObjectFilter;


	EntityFilter* parent;
	uint32_t depth;

protected:
	inline uint32_t getDepth() {
		return depth;
	}

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
	virtual void reset(EntityFilter* parent) {
		this->parent = parent;
		depth = 0;
	}

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

	inline virtual void beforeValue(Parser* parser, JsonValueType type) {
		if(getDepth() == 1) {
			for(auto e: entries) {
				if(e.idx == idx) {
					parser->enter(e.child);
					e.child->beforeValue(parser, type);
					break;
				}
			}

			idx++;
		}
	}
};

template<size_t n>
class ObjectFilter: public EntityFilter {
	template<uint16_t> friend class JsonParser;

public:
	typedef Keywords<EntityFilter*, n> Kw;
	typename Kw::Matcher matcher;
	Kw kw;

	template<class... T>
	inline ObjectFilter(T... t): kw{typename Kw::Keyword(t.name, t.child)...} {}
	inline virtual ~ObjectFilter() {}

private:
	inline virtual void reset(EntityFilter* parent) {
		EntityFilter::reset(parent);

		for(auto e: kw)
			e.getValue()->reset(this);
	}

	inline virtual void beforeKey() {
		if(getDepth() == 1)
			matcher.reset();
	}

	inline virtual void onKey(const char *at, size_t length) {
		if(getDepth() == 1)
			matcher.progress(kw, at, length);
	}

	inline virtual void beforeValue(Parser* parser, JsonValueType type) {
		if(getDepth() == 1) {
			if(const typename Kw::Keyword* result = matcher.match(kw)) {
				EntityFilter* child = result->getValue();
				parser->enter(child);
				child->beforeValue(parser, type);
			}
		}
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

class BoolExtractor: public EntityFilter {
	bool &result;

	inline virtual void onBoolean(bool value) {
		result = value;
	}

public:
	inline BoolExtractor(bool &result): result(result) {}
};

template<size_t n>
class StringExtractor: public EntityFilter {
	char (&result)[n];
	size_t offset;

	inline virtual void beforeValue(Parser* parser, JsonValueType) {
		offset = 0;
	}

	inline virtual void onString(const char *at, size_t length) {
		while(offset < n-1)
			result[offset++] = *at++;
	}

	inline virtual void afterValue(JsonValueType) {
		result[offset] = '\0';
	}

public:
	inline StringExtractor(char (&result)[n]): result(result) {}
};

template<size_t n>
inline StringExtractor<n> makeStringExtractor(char (&result)[n]) {
	return StringExtractor<n>(result);
}


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

        inline void beforeKey() {
            if(!error)
                filter->beforeKey();
        }

        inline void onKey(const char *at, size_t length) {
            if(!error)
                filter->onKey(at, length);
        }

        inline void afterKey() {
            if(!error)
                filter->afterKey();
        }

        inline void beforeValue(JsonValueType type) {
			if(!error)
				filter->beforeValue(this, type);

			filter->depth++;
        }

        inline void onNull() {
            if(!error)
                filter->onNull();
        }

        inline void onBoolean(bool x) {
            if(!error)
                filter->onBoolean(x);
        }

        inline void onNumber(int32_t value) {
            if(!error)
                filter->onNumber(value);
        }

        inline void onString(const char *at, size_t length) {
            if(!error)
                filter->onString(at, length);
        }

        inline void afterValue(JsonValueType type)
        {
			if(!error)
				filter->afterValue(type);

        	if(!--filter->depth) {
				filter = filter->parent;
			}
        }

        virtual void enter(EntityFilter *filter) {
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
            this->filter = filter;
            this->filter->reset(nullptr);
        }

        inline virtual ~JsonParser(){}
};

#endif /* JSONPARSER_H_ */
