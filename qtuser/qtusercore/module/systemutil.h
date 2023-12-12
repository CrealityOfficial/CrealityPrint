#ifndef QTUSER_CORE_SYSTEM_UTIL_H
#define QTUSER_CORE_SYSTEM_UTIL_H
#include "qtusercore/qtusercoreexport.h"
#include <QtCore/QString>

QTUSER_CORE_API void showDetailSystemInfo();
QTUSER_CORE_API void showSysMemory();
QTUSER_CORE_API int currentProcessMemory();   // K

QTUSER_CORE_API void printCallStack();

QTUSER_CORE_API QString getCanWriteFolder();
QTUSER_CORE_API void redirectIo();
QTUSER_CORE_API QString mkMutiDir(const QString& path);
QTUSER_CORE_API void mkMutiDirFromFileName(const QString& fileName);
QTUSER_CORE_API bool clearPath(const QString& path);

namespace qtuser_core
{
	QTUSER_CORE_API void removeFile(const QString& fileName);
}


namespace qtuser_core
{
	class QTUSER_CORE_API SystemUtil : public QObject
	{
		Q_OBJECT
	public:
		SystemUtil(QObject* parent = nullptr);
		virtual ~SystemUtil();

		//space  * MB
		Q_INVOKABLE int getDiskTotalSpace(const QString& driver);
		Q_INVOKABLE int getDiskFreeSpace(const QString& driver);
	};

	QTUSER_CORE_API QString calculateFileMD5(const QString& fileName);
	QTUSER_CORE_API QString calculateFileBase64(const QString& fileName);
	//16���Ƶ�md5ֵת��ΪBase64ֵ
	QTUSER_CORE_API QString convertBase64(const QString& md5);

	QTUSER_CORE_API void copyString2Clipboard(const QString& str);

	QTUSER_CORE_API int getSystemMaxPath();
}
#endif // creative_kernel_MATRIX_UTIL_H