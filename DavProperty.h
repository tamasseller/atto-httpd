/*
 * DavProperty.h
 *
 *  Created on: 2017.02.19.
 *      Author: tooma
 */

#ifndef DAVPROPERTY_H_
#define DAVPROPERTY_H_

struct DavProperty {
	const char * const xmlns;
	const char * const name;
	inline constexpr DavProperty(const char *xmlns, const char *name): xmlns(xmlns), name(name) {}
};

enum class DavAccess {
	AuthNeeded,
	NoDav,
	Dav
};

#endif /* DAVPROPERTY_H_ */
