#include "dumpproxy.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QProcess>

#include "interface/appsettinginterface.h"
#include "interface/commandinterface.h"
#include "interface/cloudinterface.h"
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
	TCHAR szMsg[MAX_PATH] = { 0 };
	SYSTEMTIME	stTime = { 0 };
	// ����dump�ļ�·��;
	GetLocalTime(&stTime);
	::GetCurrentDirectory(MAX_PATH, szDumpDir);

	QString version = creative_kernel::version();
	std::string strDumpPath = DUMP_PATH.toStdString();
	sprintf(szDumpFile, ("%s/%04d%02d%02d_%02d%02d%02d_%s_%s %s.dmp"),
		strDumpPath.c_str(),
		stTime.wYear, stTime.wMonth, stTime.wDay,
		stTime.wHour, stTime.wMinute, stTime.wSecond,
		MAIN_GIT_HASH,
		BUNDLE_NAME,
		version.toStdString().c_str());

	creative_kernel::sensorAnlyticsTrace(BUNDLE_NAME, "Crash");

	// ����dump�ļ�;
	CreateDumpFile(szDumpFile, pException);

	// ����һ������Ի��������ʾ�ϴ��� ���˳�����;
	// sprintf(szMsg, ("I'm so sorry, but the program crashed.\r\ndump file : %s"), szDumpFile);
	// FatalAppExit(1, szMsg);

	QString creality_cloud_id = QStringLiteral("cloud_%1").arg(
		creative_kernel::IsCloudLogined() ? creative_kernel::CloudUserBaseInfo().user_id : QString{});

	QDir temp_gcode_dir{ TEMPGCODE_PATH };
	temp_gcode_dir.setFilter(QDir::Filter::Files | QDir::Filter::NoSymLinks | QDir::Filter::Readable);
	QFileInfo scene_file_info{ QStringLiteral("%1/scene").arg(temp_gcode_dir.absolutePath()) };
	for (const auto& file_info : temp_gcode_dir.entryInfoList()) {
		if (!scene_file_info.exists()) {
			scene_file_info = file_info;
			continue;
		}

		if (scene_file_info.lastModified() < file_info.lastModified()) {
			scene_file_info = file_info;
		}
	}

	QString dumptool_path{ QCoreApplication::applicationDirPath() + QStringLiteral("/dumptool.exe") };
	QProcess{}.startDetached(dumptool_path, {
		QString{ szDumpFile },
		scene_file_info.absoluteFilePath(),
		std::move(creality_cloud_id)
	});

	return EXCEPTION_EXECUTE_HANDLER;
}

#endif

namespace creative_kernel
{
	DumpProxy::DumpProxy(QObject* parent)
		:QObject(parent)
	{

	}

	DumpProxy::~DumpProxy()
	{

	}

	void DumpProxy::initialize()
	{
#if _WINDOWS || WINDOWS || WIN32 || WIN64
		SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER)ApplicationCrashHandler);
#else
#endif
	}
}
