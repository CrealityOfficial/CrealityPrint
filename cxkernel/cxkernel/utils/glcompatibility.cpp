
#include <QDebug>
#include "glcompatibility.h"

#include <QtCore/QDebug>
#include <QtGui/QSurfaceFormat>
#include <QtCore/QProcess>
#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLFunctions>

#define FORCE_USE_GLES 0

namespace cxkernel
{
    bool _isNotInstalledVideoDriver() {
        QScopedPointer<QProcess> wmic(new QProcess());
        wmic->setProcessChannelMode(QProcess::MergedChannels);

        QString infName, drvName;
        QObject::connect(wmic.data(), &QProcess::readyReadStandardOutput, [&wmic, &infName, &drvName]() {
            QTextStream stream(wmic->readAllStandardOutput());
            QString s;
            while (stream.readLineInto(&s)) {
                QString line = s.trimmed();
                if (line.isEmpty())
                    continue;
                if (line.startsWith("InfFilename", Qt::CaseSensitive)) {
                    infName = line.split("=")[1];
                }
                else if (line.startsWith("Name", Qt::CaseSensitive)) {
                    drvName = line.split("=")[1];
                }
            }
            });
        wmic->start("wmic path win32_videoController get InfFilename,name /format:list");
        wmic->waitForFinished();
        return infName.startsWith("display.inf");
    }

	void setBeforeApplication()
	{
        bool noDriver = _isNotInstalledVideoDriver();
        if (noDriver)
        {
            qDebug() << "Driver isn't installed!";
            qDebug() << "Use software openGL.";
            QCoreApplication::setAttribute(Qt::AA_UseSoftwareOpenGL, true);
        }
        else 
        {
#if FORCE_USE_GLES
            qDebug() << "Use openGLES.";
            QCoreApplication::setAttribute(Qt::AA_UseOpenGLES, true);
#else
#ifdef __APPLE__
#endif
            //目前不支持ES，所以默认强制使用桌面OpenGL
            qDebug() << "Use desktop openGL.";
            QCoreApplication::setAttribute(Qt::AA_UseDesktopOpenGL, true); 
#endif
        }
	}

	uint openglVersion;
	bool isOpenGLVaild()
	{
		QOpenGLContext ctx;
		ctx.create();

		QSurfaceFormat fmt = ctx.format();
		uint actualVersion = (fmt.majorVersion() << 4) + fmt.minorVersion();
		if (!ctx.isValid() || actualVersion < openglVersion)
			return false;

		return true;
	}

	void setDefaultAfterApp()
	{
		/* 设置OpenGL版本 */
		QSurfaceFormat fmt;
		if (QOpenGLContext::openGLModuleType() == QOpenGLContext::LibGL)
		{	/* opengl */
			fmt.setVersion(3, 3);
#ifdef Q_OS_OSX
			fmt.setProfile(QSurfaceFormat::CoreProfile);
			fmt.setDepthBufferSize(24);
			fmt.setStencilBufferSize(8);
			fmt.setSamples(4);
#else
			fmt.setProfile(QSurfaceFormat::CompatibilityProfile);
			fmt.setOptions(QSurfaceFormat::DeprecatedFunctions);
#endif
		}
		else
		{	/* opengles，目前qt3d最高支持3.1 */
			fmt.setVersion(3, 1);
		}
		openglVersion = (fmt.majorVersion() << 4) + fmt.minorVersion();
		QSurfaceFormat::setDefaultFormat(fmt);

		//dynamic plugin
		QStringList dynamicPathList = QCoreApplication::libraryPaths();

		QString applicationDir = QCoreApplication::applicationDirPath();
		qDebug() << "applicationDir: " << applicationDir;

		if (!dynamicPathList.contains(applicationDir))
			dynamicPathList << applicationDir;
#ifdef Q_OS_OSX
		qDebug() << "OS OSX setDynamicLoadPath";
#ifdef QT_DEBUG
		dynamicPathList << QCoreApplication::applicationDirPath() + "/../../../";
#endif
		dynamicPathList << QCoreApplication::applicationDirPath() + "/../Frameworks";
#elif defined Q_OS_WIN32
		qDebug() << "OS WIN32 setDynamicLoadPath";
#elif defined Q_OS_LINUX
		qDebug() << "OS LINUX setDynamicLoadPath";
		dynamicPathList << applicationDir + "/lib/";
#endif

		qDebug() << "Dynamic import paths:";
		qDebug() << dynamicPathList;
		QCoreApplication::setLibraryPaths(dynamicPathList);
	}

    bool isGles()
    {
        return QCoreApplication::testAttribute(Qt::AA_UseOpenGLES);
    }

    bool isSoftwareGL()
    {
        return QCoreApplication::testAttribute(Qt::AA_UseSoftwareOpenGL);
    }
}