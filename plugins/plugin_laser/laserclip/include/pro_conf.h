#ifndef _GCORE_PROJECT_CONFIG_H_
#define _GCORE_PROJECT_CONFIG_H_

#define PLATFORM_SYSTEM     1
#define PLATFORM_MCU_ESP    2
#define DEPLOY_PLATFORM     PLATFORM_SYSTEM

#define GCODE_STYLE_GRBL    1
#define GCODE_STYLE_MARLIN  2
#define OUTPUT_GCODE_STYLE  GCODE_STYLE_MARLIN


// VERSION NUMBER:      YEAR  MONTH  MAJOR_REVISION MINOR_REVISION BUG_FIX_RELEASE
// Example:              21     09         00              01             00
#define FIRMWARE_VERSION_NUM 2109000100U

/** Version History

2109000100U     内测版（多方DLL调用测试），仅接口全部连通，功能初步完成。



********************/

#define GCORE_MAX_SUPPORTED_IMG_W   2000
#define GCORE_MAX_SUPPORTED_IMG_H   2000


#if defined(DEBUG) && defined(ANDROID)  // 安卓平台debug log

#include <android/log.h>
#define  LOG_TAG    "cxswLaser"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

#elif DEBUG // 默认Debug log

#include <stdio.h>
#define __INFO_HEADER__     "\033[0;34m[GCORE.DLL] [INFO ]\033[0m "
#define __ERROR_HEADER__    "\033[0;31m[GCORE.DLL] [ERROR]\033[0m "
#define LOGI(...) printf(__INFO_HEADER__ __VA_ARGS__)
#define LOGE(...) printf(__ERROR_HEADER__ __VA_ARGS__)

#else // 不开启 debug log

#define LOGE(...) ;
#define LOGI(...) ;

#endif

#ifdef __GNUC__
#define EXPORT_API

#elif _MSC_VER
#define EXPORT_API  extern __declspec( dllexport )
#endif

#endif // _GCORE_PROJECT_CONFIG_H_
