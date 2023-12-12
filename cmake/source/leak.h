#ifndef LEAK_1608168534436_H
#define LEAK_1608168534436_H

#ifdef _WIN32
	#if( (defined CXX_CHECK_MEMORY_LEAKS) && (defined _DEBUG) && (_MSC_VER))
		#define _CRTDBG_MAP_ALLOC
		#include <stdlib.h>
		#include <crtdbg.h>
		#define CXX_CHECK_MEMORY_LEAKS_NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
		#define new CXX_CHECK_MEMORY_LEAKS_NEW
		
		#define CXX_MEMORY_LEAK_CHECK _CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF);
		#define DUMP_MEMORY_LEAKS _CrtDumpMemoryLeaks();
		#define CXX_MEMORY_LEAKS_CHECK_ENABLED
	#else
		#define DUMP_MEMORY_LEAKS
		#define CXX_MEMORY_LEAK_CHECK
	#endif
#else
	#define DUMP_MEMORY_LEAKS
	#define CXX_MEMORY_LEAK_CHECK
#endif

#endif // LEAK_1608168534436_H