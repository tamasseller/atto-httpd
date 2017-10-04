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

	EntityFilter* const parent;
	inline EntityFilter(EntityFilter* parent): parent(parent) {}

	virtual void beforeKey(JsonParser*) = 0;
	virtual void onKey(JsonParser*, const char *at, size_t length) = 0;
	virtual void afterKey(JsonParser*) = 0;
	virtual void beforeValue(JsonParser*, JsonValueType) = 0;
	virtual void onNull(JsonParser*) = 0;
	virtual void onBoolean(JsonParser*, bool) = 0;
	virtual void onNumber(JsonParser*, int32_t value) = 0;
	virtual void onString(JsonParser*, const char *at, size_t length) = 0;
	virtual void afterValue(JsonParser*, JsonValueType) = 0;

	inline virtual ~EntityFilter() {}
};

class ArrayFilter {
	template<uint16_t> friend class JsonParser;

	struct Entry {
		uint32_t idx;
		EntityFilter* child;
	};

	inline ArrayFilter(EntityFilter* parent, EntrySet*): ArrayFilter(parent) {}

	inline virtual void beforeKey(JsonParser*) {

	}

	inline virtual void onKey(JsonParser*, const char *at, size_t length) {

	}

	inline virtual void afterKey(JsonParser*) {

	}

	inline virtual void beforeValue(JsonParser*, JsonValueType) {

	}

	inline virtual void onNull(JsonParser*) {

	}

	inline virtual void onBoolean(JsonParser*, bool) {

	}

	inline virtual void onNumber(JsonParser*, int32_t value) {

	}

	inline virtual void onString(JsonParser*, const char *at, size_t length) {

	}

	inline virtual void afterValue(JsonParser*, JsonValueType) {

	}

	inline virtual ~ArrayFilter() {}
};


template<uint16_t maxDepth>
class JsonParser: public UJson<JsonParser, maxDepth>
{
        typedef UJson<JsonParser, maxDepth> Parent;
        friend Parent;

        EntityFilter *filter;
        bool error;

        inline void beforeKey() {
            if(!error && filter)
                filter->beforeKey(this);
        }

        inline void onKey(const char *at, size_t length) {
            if(!error && filter)
                filter->onKey(this, at, length);
        }

        inline void afterKey() {
            if(!error && filter)
                filter->afterKey(this);
        }

        inline void beforeValue(JsonValueType type) {
            if(!error && filter)
                filter->beforeValue(this, type);
        }

        inline void onNull() {
            if(!error && filter)
                filter->onNull(this);
        }

        inline void onBoolean(bool x) {
            if(!error && filter)
                filter->onBoolean(this, x);
        }

        inline void onNumber(int32_t value) {
            if(!error && filter)
                filter->onNumber(this, value);
        }

        inline void onString(const char *at, size_t length) {
            if(!error && filter)
                filter->onString(this, at, length);
        }

        inline void afterValue(JsonValueType type) {
            if(!error && filter)
                filter->afterValue(this, type);
        }

        inline void onKeyError() {error = true;}
        inline void onValueError() {error = true;}
        inline void onStructureError() {error = true;}
        inline void onResourceError() {error = true;}

        union SharedState {
        	uint32_t idx;
        	KeywordMatcherState kms;
        };

    public:

        void reset(EntityFilter *filter) {
            Parent::reset();
            error = false;
            this->filter = filter;
        }
};

#endif /* JSONPARSER_H_ */
