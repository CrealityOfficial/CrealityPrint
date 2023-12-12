#ifndef _CC_GLOBAL_SPYCC_H
#define _CC_GLOBAL_SPYCC_H

#if USE_SPYCC
#define SPYCC_SESSION
#include "spycc/session.h"
#include "spycc/cmdparser.h"

#define ANALYSIS_START(x) SESSION_START(x)
#define ANALYSIS_TICK(x) SESSION_TICK(x)
#define ANALYSIS_DUMP(x) SESSION_DUMP(x)
#else
#define ANALYSIS_START(x) void(x)
#define ANALYSIS_TICK(x) void(x)
#define ANALYSIS_DUMP(x) void(x)
#define SESSION_START(x) void(x)
#define SESSION_TICK(x) void(x)
#define SESSION_DUMP(x) void(x)
#endif

#endif