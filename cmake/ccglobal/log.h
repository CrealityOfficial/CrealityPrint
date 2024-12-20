#ifndef __CC_GLOBAL_LOG__
#define __CC_GLOBAL_LOG__

// log level
// cxlog_verbose = 0,
// cxlog_debug = 1,
// cxlog_info = 2,
// cxlog_warn = 3,
// cxlog_err = 4,
// cxlog_critical = 5
// cxlog_main = 6

#if CC_USE_SPDLOG
#include "spdlog/cxlog_macro.h"

	#define LOGV(...) CXLogVerbose(__VA_ARGS__)
	#define LOGVID(logSortId,...) CXLogVerboseID(logSortId,##__VA_ARGS__)
	#define LOGD(...) CXLogDebug(__VA_ARGS__)
	#define LOGDID(logSortId,...) CXLogDebugID(logSortId,##__VA_ARGS__)
	#define LOGI(...) CXLogInfo(__VA_ARGS__)
	#define LOGIID(logSortId,...) CXLogInfoID(logSortId,##__VA_ARGS__)
	#define LOGW(...) CXLogWarn(__VA_ARGS__)
    #define LOGWID(logSortId,...) CXLogWarnID(logSortId,##__VA_ARGS__)
	#define LOGE(...) CXLogError(__VA_ARGS__)
    #define LOGEID(logSortId,...) CXLogErrorID(logSortId,##__VA_ARGS__)
	#define LOGC(...) CXLogCritical(__VA_ARGS__)
	#define LOGCID(logSortId,...) CXLogCriticalID(logSortId,##__VA_ARGS__)
	#define LOGM(...) CXLogMain(__VA_ARGS__)
	#define LOGMID(logSortId,...) CXLogMainID(logSortId,##__VA_ARGS__)
	
    #define LOGINIT(x) cxlog::CXLog::Instance().InitCXLog(x)
	#define LOGDIR(x) cxlog::CXLog::Instance().setDirectory(x)
	#define LOGLEVEL(x) cxlog::CXLog::Instance().SetLevel(x)
	#define LOGEND()  cxlog::CXLog::Instance().EndLog()
    #define LOGCONSOLE() cxlog::CXLog::Instance().setColorConsole()
	#define LOGNAMEFUNC(x) cxlog::CXLog::Instance().setNameFunc(x)
#else
	#if __ANDROID__
		#include <android/log.h>
		 
		#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE,"NativeCC",__VA_ARGS__)
		#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG,"NativeCC",__VA_ARGS__)
		#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,"NativeCC",__VA_ARGS__)
		#define LOGW(...) __android_log_print(ANDROID_LOG_WARN,"NativeCC",__VA_ARGS__)
		#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,"NativeCC",__VA_ARGS__)
		#define LOGC(...) __android_log_print(ANDROID_LOG_ASSERT,"NativeCC",__VA_ARGS__)
		#define LOGM(...) __android_log_print(ANDROID_LOG_ERROR,"NativeCC",__VA_ARGS__)
	#else
		#include <stdarg.h>
		#include <stdio.h>
		#include <iostream>
		
		#define LOGV(...) printf(__VA_ARGS__);printf("\n")
		#define LOGD(...) printf(__VA_ARGS__);printf("\n")
		#define LOGI(...) printf(__VA_ARGS__);printf("\n")
		#define LOGW(...) printf(__VA_ARGS__);printf("\n")
		#define LOGE(...) printf(__VA_ARGS__);printf("\n")
		#define LOGC(...) printf(__VA_ARGS__);printf("\n")
		#define LOGM(...) printf(__VA_ARGS__);printf("\n")
	#endif

	#define LOGVID(logSortId,...) (void)0
	#define LOGDID(logSortId,...) (void)0
	#define LOGIID(logSortId,...) (void)0
	#define LOGWID(logSortId,...) (void)0
	#define LOGEID(logSortId,...) (void)0
	#define LOGCID(logSortId,...) (void)0
	#define LOGMID(logSortId,...) (void)0

	#define LOGDIR(x) (void)0 
	#define LOGLEVEL(x) (void)0
	#define LOGEND() (void)0
    #define LOGCONSOLE() (void)0
	#define LOGNAMEFUNC(x) (void)0
#endif

#endif // __CC_GLOBAL_LOG__