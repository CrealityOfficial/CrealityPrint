#include <fstream>

#include <QtCore/QCryptographicHash>
#include <QtCore/QObject>
#include <QtCore/QTranslator>
#include <QtGui/QGuiApplication>
#include <QtNetwork/QHostInfo>
#include <QtQml/QQmlApplicationEngine>
#include <QtQml/QQmlContext>

#include <buildinfo.h>

#include <qtusercore/util/settings.h>

#include "dumptool.h"
#include "wizardui.hpp"

/// argv example: [
///   "C:/Users/xxx/AppData/Roaming/Creality/Creative3D/4.3/Log/xxx.dmp", // dump file abs path
///   "C:/Users/xxx/AppData/Roaming/Creality/Creative3D/0.0/GCodes/test", // scene file abs path
///   "cloud_xxx", // creality cloud id (empty if not login)
/// ]

int main(int argc, char* argv[]) {
  QGuiApplication application{ argc, argv };

  // argument vaild check
  QStringList argument_list = QCoreApplication::arguments();
  if (argument_list.size() != 4 || !std::ifstream{ argument_list[1].toStdString() }.is_open()) {
    return -1;
  }

  QCoreApplication::setApplicationName(PROJECT_NAME);
  QCoreApplication::setApplicationVersion(PROJECT_VERSION);
  QCoreApplication::setOrganizationName(ORGANIZATION);

  QString creality_cloud_id = argument_list[3];
  if (creality_cloud_id == QStringLiteral("cloud_")) {
    auto user_id = QCryptographicHash::hash(QHostInfo::localHostName().toUtf8(),
      QCryptographicHash::Algorithm::Md5).toHex();
    creality_cloud_id = QStringLiteral("local_%1").arg(QString{ std::move(user_id) });
  }

  DumpTool dump_tool{ argument_list[1], argument_list[2], creality_cloud_id };

  // init langauge
  qtuser_core::VersionSettings setting;
  setting.beginGroup(QStringLiteral("language_perfer_config"));
  QString language_file_name = setting.value(
    QStringLiteral("language_type"), QStringLiteral("en.ts"))
      .toString().replace(QStringLiteral(".ts"), QStringLiteral(".qm"));
  setting.endGroup();
  QTranslator translater;
  translater.load(QStringLiteral(":/language/%1").arg(language_file_name));
  QCoreApplication::installTranslator(&translater);

  // init theme
  setting.beginGroup("themecolor_config");
  int theme_index = setting.value("themecolor_config", 0).toInt();
  setting.endGroup();

  // open window
  WizardUi wizard_ui;
  QQmlApplicationEngine engine;
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
    { QStringLiteral("screenScaleFactor"), QVariant::fromValue(wizard_ui.getScreenScaleFactor()) },
    { QStringLiteral("WizardUI"), QVariant::fromValue(&wizard_ui) },
    { QStringLiteral("dumpTool"), QVariant::fromValue(&dump_tool) },
  });
  engine.load(QStringLiteral("qrc:/ui/main.qml"));

  return QCoreApplication::exec();
}
