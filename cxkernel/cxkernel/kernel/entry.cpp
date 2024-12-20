#include "entry.h"
#include <QtWidgets/QApplication>
#include <QResource>
#include <QtCore/QDebug>
#include <QtQml/QQmlApplicationEngine>
#include <QtQml/QQmlContext>
#include <QtCore/QDateTime>
#include <QGuiApplication>
#include <QOpenGLContext>
#include <QtQuick/QQuickView>
#include <QFont>
#include "qtusercore/module/systemutil.h"
#include "cxkernel/utils/glcompatibility.h"
#include "frameless/FrameLessView.h"
#include <QIcon>
#include <locale.h>

#include "ccglobal/log.h"
#include "buildinfo.h"
#include "qtusercore/string/resourcesfinder.h"
#include <QTextCodec>
#include <QFontMetrics>
#ifndef Q_OS_WIN
	#include <sys/resource.h>
#endif
namespace cxkernel
{
	void outputMessage(QtMsgType type, const QMessageLogContext& context, const QString& msg)
	{
#ifdef QT_NO_DEBUG
		QString text = QString("[%3]").arg(msg);
#else
#if DEBUG_FUNCTION
		QString text = QString("[FILE %1, FUNCTION %2][%3]").arg(context.file).arg(context.function).arg(msg);
#else
		QString text = QString("[%3]").arg(msg);
#endif
#endif
		switch (type)//log ��Ϣ����
		{
#ifdef QT_DEBUG
		case QtDebugMsg:
			LOGD("%s", text.toLocal8Bit().constData());
			break;
#endif
		case QtInfoMsg:
			LOGI("%s", text.toLocal8Bit().constData());
			break;
		case QtWarningMsg:
			LOGW("%s", text.toLocal8Bit().constData());
			break;
		case QtCriticalMsg:
			LOGM("%s", text.toLocal8Bit().constData());
			break;
		case QtFatalMsg:
			LOGC("%s", text.toLocal8Bit().constData());
			break;
		default:
			LOGV("%s", text.toLocal8Bit().constData());
			break;
	}
}

	void initializeLog(int argc, char* argv[])
	{


		QCoreApplication::setOrganizationName(QStringLiteral(ORGANIZATION));
    	QCoreApplication::setOrganizationDomain(QStringLiteral("FDM"));
#ifdef Q_OS_LINUX
    	QCoreApplication::setApplicationName(
      	QStringLiteral(BUNDLE_NAME).replace(QStringLiteral("_"), QStringLiteral(" ")));
#else
    	QCoreApplication::setApplicationName(QStringLiteral("Creative3D"));
#endif

		QString logDirectory = qtuser_core::getOrCreateAppDataLocation("Log");

		auto func = [](const char* name)->std::string {
			QString  dataTime = QDateTime::currentDateTime().toString("yyyy-MM-dd");
			dataTime += QString(".cxlog");
			return dataTime.toLocal8Bit().data();
		};

		LOGNAMEFUNC(func);
		LOGDIR(logDirectory.toLocal8Bit().data());
#ifdef QT_DEBUG
		LOGCONSOLE();
#endif
		LOGLEVEL(1);

		//qInstallMessageHandler(outputMessage);

		qDebug() << QString("----------> START LOG <-----------");
	}

	void uninitializeLog()
	{
		qDebug() << QString("----------> END LOG <-----------");
		LOGEND();
	}

	void setDefaultBeforApp()
	{
#ifdef Q_OS_OSX
		QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif

		//dynamic plugin
		QStringList dynamicPathList = QCoreApplication::libraryPaths();

#ifdef Q_OS_OSX
		qDebug() << "OS OSX pre setDynamicLoadPath";
#elif defined Q_OS_WIN32
		qDebug() << "OS WIN32 pre setDynamicLoadPath";
#elif defined Q_OS_LINUX
		qDebug() << "OS LINUX pre setDynamicLoadPath";

		if (qEnvironmentVariableIsSet("APPDIR"))
		{
			QString appdir = qEnvironmentVariable("APPDIR");
			qDebug() << "Linux get the APPDIR : " << appdir;
			dynamicPathList << appdir + "/plugins";
		}
#endif

		qDebug() << "Pre Dynamic import paths:";
		qDebug() << dynamicPathList;
		QCoreApplication::setLibraryPaths(dynamicPathList);
	}

    void setDefaultQmlBeforeApp()
    {

    }

	void setDefaultQmlAfterApp(QQmlEngine& engine)
	{
		//qml plugin
		QStringList qmlPathList = engine.importPathList();

		QString applicationDir = QCoreApplication::applicationDirPath();
		qDebug() << "applicationDir " << applicationDir;

#ifdef Q_OS_OSX
		qDebug() << "OS OSX setQmlLoadPath";
		qmlPathList << applicationDir + "/../Resources/qml";
#elif defined Q_OS_WIN32
		qDebug() << "OS WIN32 setQmlLoadPath";
#elif defined Q_OS_LINUX
		qDebug() << "OS LINUX setQmlLoadPath";
		qmlPathList << applicationDir + "/lib/";
		qmlPathList << applicationDir + "/qml/";
#endif

		qDebug() << "Qml import paths:";
		qDebug() << qmlPathList;

		engine.setImportPathList(qmlPathList);
	}
	float getScreenScaleFactor()
	{
#ifdef __APPLE__
		QFont font = qApp->font();
		font.setPointSize(12);
		qApp->setFont(font);
		return 1;
#else
		QFont font = qApp->font();
		font.setPointSize(10);
		qApp->setFont(font);
		//int ass = QFontMetrics(font).ascent();
		float fontPixelRatio = QFontMetrics(qApp->font()).ascent() / 11.0;
		fontPixelRatio = int(fontPixelRatio * 4) / 4.0;
		return fontPixelRatio;
#endif
	}
	int appMain(int argc, char* argv[], AppModuleCreateFunc func)
	{
		initializeLog(argc, argv);

		setDefaultBeforApp();
		setBeforeApplication();
		setDefaultQmlBeforeApp();

		// Will make Qt3D and QOpenGLWidget share a common context
		QApplication::setAttribute(Qt::AA_ShareOpenGLContexts);

		QApplication app(argc, argv);
		setlocale(LC_NUMERIC, "C");
#ifndef Q_OS_WIN
		struct rlimit limit;
		if (getrlimit(RLIMIT_NOFILE, &limit) == 0) {
			qWarning() << "Current limits: soft=" << limit.rlim_cur << " hard=" << limit.rlim_max ;

			// 设置新的软限制和硬限制
			limit.rlim_cur = 4096; // 可根据需要设置新的软限制
			// 硬限制通常需要 root 权限才能更改
			//limit.rlim_max = 4096; 
			if (setrlimit(RLIMIT_NOFILE, &limit) != 0) {
				qWarning()  << "Failed to set new limits" ;
			}
		} else {
			qWarning()  << "Failed to get current limits" ;
		}
#endif
#ifdef Q_OS_WIN
		if (GetACP() != GetOEMCP())
		{
			QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
		}

#endif // Q_OS_WIN
		QString rcc = qApp->applicationDirPath() + "/CrealityUIRes.rcc";
    #ifdef Q_OS_OSX
        rcc = qApp->applicationDirPath() + "/../Resources/CrealityUIRes.rcc";
    #endif
    #ifdef Q_OS_LINUX
    #endif
		if(QResource::registerResource(rcc))
		{
			qDebug() << "success";
		}
		app.setWindowIcon(QIcon(":/scence3d/res/logo.png"));

#ifdef Q_OS_OSX
		QQuickWindow::setTextRenderType(QQuickWindow::NativeTextRendering);
#endif

		AppModule* appModule = func ? func() : nullptr;
		int ret = 0;
		if (appModule)
		{
			{
				bool useFrameless = appModule->useFrameless();

				QFont default_font = QFont();
				default_font.setPointSize(9);
				app.setFont(default_font);

				QQmlEngine* engine = nullptr;
				QObject* object = nullptr;
				if (useFrameless)
				{
//#ifdef Q_OS_OSX
					setDefaultAfterApp();
//#endif
					FrameLessView* view = new FrameLessView();
					view->setMinimumSize({ static_cast<int>(1280* getScreenScaleFactor()), static_cast<int>(720* getScreenScaleFactor()) });
					view->setColor(QColor("transparent"));
					view->setTitle(QStringLiteral(BUNDLE_NAME).replace(QStringLiteral("_"), QStringLiteral(" ")));
					engine = view->engine();
					QObject::connect(engine,&QQmlEngine::quit,[&](){QCoreApplication::quit();});
					object = view;
//#ifdef Q_OS_WIN
//						setDefaultAfterApp();
//#endif
				}
				else
				{
					QQmlApplicationEngine* qEngine = new QQmlApplicationEngine();
					engine = qEngine;
					object = qEngine;
					setDefaultAfterApp();
				}

				setDefaultQmlAfterApp(*engine);

				showDetailSystemInfo();

				if (appModule->loadQmlEngine(object, *engine))
				{
					if (useFrameless)
						qobject_cast<FrameLessView*>(object)->showMaximized();
					ret = app.exec();
				}

				appModule->unloadQmlEngine();

				delete object;
			}
			appModule->shutDown();
			delete appModule;
		}

		uninitializeLog();
		return ret;
	}
}
