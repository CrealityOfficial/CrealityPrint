#ifndef QEXPORT_INTERFACE_1604911737496_H
#define QEXPORT_INTERFACE_1604911737496_H
#include <QtCore/QObject>

#ifdef WIN32
	#define CC_DECLARE_EXPORT __declspec(dllexport)
	#define CC_DECLARE_IMPORT __declspec(dllimport)
	#define CC_DECLARE_STATIC 
#else
	#define CC_DECLARE_EXPORT __attribute__((visibility("default")))
	#define CC_DECLARE_IMPORT
	#define CC_DECLARE_STATIC
#endif

#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
#define Q515 1
#else
#define Q515 0
#endif

//  example for dll/lib declare
// 
//	#if USE_DLLA_DLL
//		#define DLLA_API CC_DECLARE_IMPORT
//	#elif USE_DLLA_STATIC
//		#define DLLA_API CC_DECLARE_STATIC
//	#else
//		#if DLLA_DLL
//			#define DLLA_API CC_DECLARE_EXPORT
//		#else
//			#define DLLA_API CC_DECLARE_STATIC
//		#endif
//	#endif
#endif // QEXPORT_INTERFACE_1604911737496_H