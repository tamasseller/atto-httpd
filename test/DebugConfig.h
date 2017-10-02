/*
 * DebugConfig.h
 *
 *      Author: tamas.seller
 */

#ifndef DEBUGCONFIG_H_
#define DEBUGCONFIG_H_

#include "ubiquitous/ConfigHelper.h"

#include "ubiquitous/PrintfWriter.h"

GLOBAL_TRACE_POLICY(All);
TRACE_WRITER(PrintfWriter);

#endif /* DEBUGCONFIG_H_ */
