/*
 * JsonParser.h
 *
 *      Author: tamas.seller
 */

#ifndef JSONPARSER_H_
#define JSONPARSER_H_

/*
 * Hierarchical JSON filtering.
 *
 * This module extracts values from JSON documents according to a
 * user supplied filter structure. It processes the document using
 * the _UJson_ low-level parser that converts the source document
 * into a sequence of 'before', 'on' and 'after' function calls
 * according to the contents of the source document.
 *
 * The filters are organized in a hierarchical fashion similar to
 * the actual document format. There is always a single active
 * filter at any time, that receives the parser events. The active
 * filter can decide to pass control when receives an event signifying
 * the start of a new value to another filter that is known as its
 * child. The child is automatically terminated when the ending
 * event of the same entity is received as it was given control at.
 *
 * In this fashion a filter structure that mimics the hierarchy of
 * some part of the expected document structure can be used to extract
 * just those portions.
 *
 * During the operation of the filtering mechanism, no document data
 * other than the matched results are copied at any time.
 */

#include "UJson.h"
#include "Keywords.h"

#include "algorithm/Str.h"

/**
 * Base class and parser facing interface for the filter workers.
 *
 * Contains a pointer to the parent and a depth counter, both of
 * which are managed by the _JsonParser_.
 *
 * The parent is the filter that selected this one, and the counter
 * is incremented upon entry to a nested object or array and
 * decremented when exiting.
 */
class EntityFilter {
	template<uint16_t> friend class JsonParser;
	template<size_t> friend class ArrayFilter;
	template<size_t> friend class ObjectFilter;

	/**
	 * The previously active filter.
	 *
	 * This is needed to be stored here in order to enable the
	 * parser to return control to the parent filter, upon
	 * receiving the closing event of the entity that initiated
	 * this filter.
	 */
	EntityFilter* parent;

	/**
	 * Entity nesting counter.
	 *
	 * Counter to keep track of nesting of document entities under
	 * the current filter. It starts as zero before starting the
	 * operation of the filter, and is immediately incremented by
	 * the _beforeValue_ belonging to the entity that have triggered
	 * this filter. It is incremented for every further _beforeValue_
	 * event and decremented for _afterValue_. The counter keeps its
	 * value while a child is active.
	 *
	 * When the counter reaches zero, it is known that the triggering
	 * element is completely processed, and the _JsonParser_ returns
	 * control to the parent.
	 */
	uint32_t depth;

protected:
	/**
	 * Depth counter accessor for children.
	 */
	inline uint32_t getDepth() {
		return depth;
	}

	/**
	 * Interface for filter activation.
	 *
	 * This is intended to be implemented by the _JsonParser_ and
	 * enables the filters to activate their children.
	 */
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
	virtual void onParentLeave() {}

public:
	/**
	 * Reset function, to be called before processing a new document.
	 */
	virtual void reset(EntityFilter* parent) {
		this->parent = parent;
		depth = 0;
	}

	inline virtual ~EntityFilter() {}
};

/**
 * Filter parameter holder.
 *
 * This is a convenience structure to enable easy passing of
 * parameters for both the array and object filters.
 */
class FilterEntry {
		template<size_t> friend class ArrayFilter;
		template<size_t> friend class ObjectFilter;

		union {
			uint32_t idx;
			const char* name;
		};

		EntityFilter* child;

	public:
		/**
		 * Create array filter entry.
		 *
		 * The _child_ is triggered when reaching the value at
		 * the _idx_ (zero-based) position.
		 */
		FilterEntry (uint32_t idx, EntityFilter* child): idx(idx), child(child) {}

		/**
		 * Create object filter entry.
		 *
		 * The _child_ is triggered when reaching the value belonging
		 * to the _name_  name (null-terminated string).
		 */
		FilterEntry (const char* name, EntityFilter* child): name(name), child(child) {}
};

/**
 * Filter worker for selecting elements of an array identified by their indices.
 *
 * The _n_ parameter is the number of elements that can be selected.
 */
template<size_t n>
class ArrayFilter: public EntityFilter {
	template<uint16_t> friend class JsonParser;

public:
	/// Child entries.
	FilterEntry entries[n];

	/**
	 * Index of the current element.
	 *
	 * This is incremented for each _beforeValue_ that belongs
	 * to the immediate contents of the array.
	 */
	uint32_t idx;

	template<class... T>
	inline ArrayFilter(T... t): entries{t...} {}
	inline virtual ~ArrayFilter() {}

private:
	/// Reset the common parser state, also reset children and the index counter.
	inline virtual void reset(EntityFilter* parent) {
		EntityFilter::reset(parent);
		idx = 0;

		for(auto e: entries)
			e.child->reset(this);
	}

	/**
	 * Keeps track of the current element index, enters child on match.
	 */
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

/**
 * Filter worker for selecting members of an object identified by their names.
 *
 * The _n_ parameter is the number of members that can be selected.
 */
template<size_t n>
class ObjectFilter: public EntityFilter {
	template<uint16_t> friend class JsonParser;

public:
	/**
	 * _Keywords_ matecher based on the specified names.
	 */
	typedef Keywords<EntityFilter*, n> Kw;
	typename Kw::Matcher matcher;
	Kw kw;

	template<class... T>
	inline ObjectFilter(T... t): kw{typename Kw::Keyword(t.name, t.child)...} {}
	inline virtual ~ObjectFilter() {}

private:
	/// Reset the common parser state, also reset children.
	inline virtual void reset(EntityFilter* parent) override {
		EntityFilter::reset(parent);

		for(auto e: kw)
			e.getValue()->reset(this);
	}

	/// Reset the keyword matcher.
	inline virtual void beforeKey() {
		if(getDepth() == 1)
			matcher.reset();
	}

	/// Update the keyword matcher.
	inline virtual void onKey(const char *at, size_t length) override {
		if(getDepth() == 1)
			matcher.progress(kw, at, length);
	}

	/// Check for a keyword match, enter children if needed.
	inline virtual void beforeValue(Parser* parser, JsonValueType type) override {
		if(getDepth() == 1) {
			if(const typename Kw::Keyword* result = matcher.match(kw)) {
				EntityFilter* child = result->getValue();
				parser->enter(child);
				child->beforeValue(parser, type);
			}
		}
	}

	inline virtual void afterValue(JsonValueType) override
	{
		for(const auto &k : kw)
			k.getValue()->onParentLeave();
	}
};

/// Leaf filter to extract numeric value.
class NumberExtractor: public EntityFilter {
	/// Reference to the output storage.
	int &result;

	/// Write output when number is received.
	inline virtual void onNumber(int32_t value) override {
		result = value;
	}
public:
	inline NumberExtractor(int &result): result(result) {}
};

/// Leaf filter to extract boolean value.
class BoolExtractor: public EntityFilter {
	/// Reference to the output storage.
	bool &result;

	/// Write output when boolean is received.
	inline virtual void onBoolean(bool value) override {
		result = value;
	}

public:
	inline BoolExtractor(bool &result): result(result) {}
};

/**
 * Leaf filter to extract string value.
 *
 * This extractor expects a fixed size array that it
 * can copy the data into.
 *
 * The _n_ parameter is the length of the output array.
 */
template<size_t n>
class StringExtractor: public EntityFilter {
	/// Reference to the output storage.
	char (&result)[n];

	/// Offset of the next free byte into the output storage.
	size_t offset;

	/// Reset the offset counter to the start of the output storage.
	inline virtual void beforeValue(Parser* parser, JsonValueType) override {
		offset = 0;
	}

	/// Copy data to the end of the output.
	inline virtual void onString(const char *at, size_t length) override {
		while(offset < n-1)
			result[offset++] = *at++;
	}

	/// Null-terminate output.
	inline virtual void afterValue(JsonValueType) override {
		result[offset] = '\0';
	}

public:
	inline StringExtractor(char (&result)[n]): result(result) {}
};

/**
 * Main filter executor.
 *
 * Keeps track of the current filter, and error state. When a lower
 * level (_UJson_) parse error is encountered stops processing of data.
 *
 * Forwards events to the active filter and manages the _depth_ counter
 * and control returning of the active filter based on that.
 */
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

        virtual void enter(EntityFilter *filter) override {
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


/// Convenience factory like method to automatically deduce target array size.
template<size_t n>
inline StringExtractor<n> makeStringExtractor(char (&result)[n]) {
	return StringExtractor<n>(result);
}

/// Convenience factory like method to automatically deduce number of children.
template<template<size_t> class T, class... A>
T<sizeof...(A)> assemble(A... a) {
	return T<sizeof...(A)>(a...);
}

#endif /* JSONPARSER_H_ */
