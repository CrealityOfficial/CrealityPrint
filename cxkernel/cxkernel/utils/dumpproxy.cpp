#include "dumpproxy.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QProcess>

#include "cxkernel/kernel/cxkernel.h"
#include "buildinfo.h"

#if _WINDOWS || WINDOWS || WIN32 || WIN64

#include <Windows.h>
#include <DbgHelp.h>

void CreateDumpFile(TCHAR* strPath, EXCEPTION_POINTERS* pException)
{
	// ����Dump�ļ�;
	HANDLE hDumpFile = CreateFile(strPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	// Dump��Ϣ;
	MINIDUMP_EXCEPTION_INFORMATION dumpInfo;
	dumpInfo.ExceptionPointers = pException;
	dumpInfo.ThreadId = GetCurrentThreadId();
	dumpInfo.ClientPointers = TRUE;

	// д��Dump�ļ�����;
	MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hDumpFile, MiniDumpNormal, &dumpInfo, NULL, NULL);
	CloseHandle(hDumpFile);
}

LONG ApplicationCrashHandler(EXCEPTION_POINTERS* pException)
{
	TCHAR szDumpDir[MAX_PATH] = { 0 };
	TCHAR szDumpFile[MAX_PATH] = { 0 };
	SYSTEMTIME	stTime = { 0 };
	// ����dump�ļ�·��;
	GetLocalTime(&stTime);
	::GetCurrentDirectory(MAX_PATH, szDumpDir);

	cxkernel::DumpProxy* proxy = cxkernel::cxKernel->dumpProxy();

	QString version = proxy->version();
	QString path = proxy->dumpPath();
	std::string strDumpPath = path.toStdString();
	sprintf(szDumpFile, ("%s/%04d%02d%02d_%02d%02d%02d_%s %s.dmp"),
		strDumpPath.c_str(),
		stTime.wYear, stTime.wMonth, stTime.wDay,
		stTime.wHour, stTime.wMinute, stTime.wSecond,
		PROJECT_NAME,
		version.toStdString().c_str());

	//creative_kernel::sensorAnlyticsTrace("Creality Print", "Crash");

	CreateDumpFile(szDumpFile, pException);

	QString creality_cloud_id = proxy->cloudId();

	QString dumptool_path{ QCoreApplication::applicationDirPath() + QStringLiteral("/dumptool.exe") };
	QProcess{}.startDetached(dumptool_path, { QString{ szDumpFile }, std::move(creality_cloud_id) });

	return EXCEPTION_EXECUTE_HANDLER;
}

#endif 

namespace cxkernel
{
	DumpProxy::DumpProxy(QObject* parent)
		:QObject(parent)
	{

	}

	DumpProxy::~DumpProxy()
	{

	}

	void DumpProxy::initialize(const QString& version, const QString& cloudId, const QString& path)
	{
		m_version = version;
		m_cloudId = cloudId;
		m_path = path;

#if _WINDOWS || WINDOWS || WIN32 || WIN64
		SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER)ApplicationCrashHandler);
#else
#endif
	}


	QString DumpProxy::cloudId()
	{
		return m_cloudId;
	}

	QString DumpProxy::dumpPath()
	{
		return m_path;
	}

	QString DumpProxy::version()
	{
		return m_version;
	}
}
