#include <fstream>

#include <QQmlApplicationEngine>
#include <QGuiApplication>
#include <QQmlContext>
#include <QTranslator>
#include <QCryptographicHash>
#include <QHostInfo>
#include <QSettings>
#include <QObject>
#include "buildinfo.h"
#include "unittesttool.h"

/// argv example: [
///   "C:/Users/xxx/AppData/Roaming/HalotBox/xxx.dmp",  // dump file abs path
///   "cloud_xxx", // creality cloud id (empty if not login)
/// ]

int main(int argc, char* argv[]) {
    
  QGuiApplication application{ argc, argv };
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
  QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
  QCoreApplication::setOrganizationName(QStringLiteral(ORGANIZATION));
  QCoreApplication::setOrganizationDomain(QStringLiteral("FDM"));
  QCoreApplication::setApplicationName(QStringLiteral("UnitTestTool"));
  UnitTestTool _testTool;

  // init langauge
  QString language_file_name = "en.ts";
  QTranslator translater;
  translater.load(QStringLiteral(":/language/%1").arg(language_file_name));
  QCoreApplication::installTranslator(&translater);

  _testTool.initTranslator();

  int theme_index = 0;

  QQmlApplicationEngine engine;

  _testTool.setQmlEngine(&engine);
  engine.rootContext()->setContextProperties({
    { QStringLiteral("defaultTheme"), QVariant::fromValue(theme_index) },
    {
      QStringLiteral("fontsPath"),
#ifdef Q_OS_OSX
      QCoreApplication::applicationDirPath() + QStringLiteral("/../Resources/resources/fonts/")
#else
      QCoreApplication::applicationDirPath() + QStringLiteral("/resources/fonts/")
#endif
    },
    { QStringLiteral("screenScaleFactor"), QVariant::fromValue(1) },
    { QStringLiteral("gTestTool"), QVariant::fromValue(&_testTool) },
  });
  engine.load(QStringLiteral("qrc:/ui/main.qml"));


  return application.exec();
}
