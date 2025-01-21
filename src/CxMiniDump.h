#ifndef CxMiniDump_hpp_
#define CxMiniDump_hpp_
#pragma once
#include <Windows.h>
#include <tchar.h>
#include <DbgHelp.h>
//#pragma comment(lib, "dbghelp.lib")

#ifdef UNICODE
#define TSprintf	wsprintf
#else
#define TSprintf	sprintf
#endif
#include <string>
#include <sstream>
#include <iostream>
class MiniDump
{
private:
	MiniDump();
	~MiniDump();
	
public:
	// 程序崩溃时是否启动自动生成dump文件;
	// 只需要在main函数开始处调用该函数即可;
	static void EnableAutoDump(bool bEnable = true);
	static std::string dumpDir();
private:

	static LONG ApplicationCrashHandler(EXCEPTION_POINTERS* pException);
	static void CreateDumpFile(LPCWSTR lpstrDumpFilePathName, EXCEPTION_POINTERS* pException);
};

#endif	