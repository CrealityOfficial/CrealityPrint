#include "kernel/globalconst.h"

#include <fstream>

#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QSettings>
#include <QtCore/QStandardPaths>

#include <rapidjson/document.h>
#include <rapidjson/filereadstream.h>

#include <buildinfo.h>

#include <cxcloud/service_center.h>

#include <cxgcode/us/settingdef.h>

#include <qtusercore/module/systemutil.h>
#include <qtusercore/string/resourcesfinder.h>

#include "external/interface/cloudinterface.h"
#include "internal/parameter/parameterpath.h"
#include "kernel/kernel.h"

// #pragma execution_character_set("utf-8")

namespace creative_kernel {

  GlobalConst::GlobalConst(QObject* parent)
      : cxkernel::CXKernelConst{ parent }
      , m_lanTsFiles{
          QStringLiteral("en.ts"),
          QStringLiteral("zh_CN.ts"),
          QStringLiteral("zh_TW.ts"),
          QStringLiteral("ko.ts"),
        }
      , m_lanNames{
          QStringLiteral("English"),
          QStringLiteral("简体中文"),
          QStringLiteral("繁體中文"),
          QStringLiteral("한국어/Korean")
        } {
    QCoreApplication::setOrganizationName(QStringLiteral(ORGANIZATION));
    QCoreApplication::setOrganizationDomain(QStringLiteral("FDM"));
#ifdef Q_OS_LINUX
    QCoreApplication::setApplicationName(
      QStringLiteral(BUNDLE_NAME).replace(QStringLiteral("_"), QStringLiteral(" ")));
#else
    QCoreApplication::setApplicationName(QStringLiteral(PROJECT_NAME));
#endif

    initialize();
  }

  bool GlobalConst::isCustomized() const {
#ifndef CUSTOMIZED
    return false;
#else
    return true;
#endif  // !CUSTOMIZED
  }

  bool GlobalConst::isCxcloudEnabled() const {
#ifndef CUSTOMIZED
    return true;
#else
#  ifdef CUSTOM_CXCLOU_ENABLED
    return true;
#  else
    return false;
#  endif  // CUSTOM_CXCLOU_ENABLED
#endif  // !CUSTOMIZED
  }

  bool GlobalConst::isUpgradeable() const {
#ifndef CUSTOMIZED
    return true;
#else
#  ifndef CUSTOM_UN_UPGRADEABLE
    return true;
#  else
    return false;
#  endif  // !CUSTOM_UN_UPGRADEABLE
#endif  // !CUSTOMIZED
  }

  bool GlobalConst::isFeedbackable() const {
#ifndef CUSTOMIZED
    return true;
#else
#  ifndef CUSTOM_UN_FEEDBACKABLE
    return true;
#  else
    return false;
#  endif  // !CUSTOM_UN_FEEDBACKABLE
#endif  // !CUSTOMIZED
  }

  bool GlobalConst::isTeachable() const {
#ifndef CUSTOMIZED
    return true;
#else
#  ifndef CUSTOM_UN_TEACHABLE
    return true;
#  else
    return false;
#  endif  // !CUSTOM_UN_TEACHABLE
#endif  // !CUSTOMIZED
  }

  bool GlobalConst::isHollowEnabled() const {
#ifndef CUSTOMIZED
    return true;
#else
#  ifdef CUSTOM_HOLLOW_ENABLED
    return true;
#  else
    return false;
#  endif  // !CUSTOM_HOLLOW_ENABLED
#endif  // !CUSTOMIZED
  }

  bool GlobalConst::isHollowFillEnabled() const {
#ifndef CUSTOMIZED
    return false;
#else
#  ifdef CUSTOM_HOLLOW_FILL_ENABLED
    return true;
#  else
    return false;
#  endif  // !CUSTOM_HOLLOW_FILL_ENABLED
#endif  // !CUSTOMIZED
  }

  bool GlobalConst::isDrillable() const {
#ifndef CUSTOMIZED
    return true;
#else
#  ifndef CUSTOM_UN_DRILLABLE
    return true;
#  else
    return false;
#  endif  // !CUSTOM_UN_DRILLABLE
#endif  // !CUSTOMIZED
  }

  bool GlobalConst::isDistanceMeasureable() const {
#ifndef CUSTOMIZED
    return true;
#else
#  ifndef CUSTOM_UN_DRILLABLE
    return true;
#  else
    return false;
#  endif  // !CUSTOM_UN_DRILLABLE
#endif  // !CUSTOMIZED
  }

  bool GlobalConst::isSliceHeightSettingEnabled() const {
#ifndef CUSTOMIZED
    return false;
#else
#  ifdef CUSTOM_SLICE_HEIGHT_SETTING_ENABLED
    return true;
#  else
    return false;
#  endif  // CUSTOM_SLICE_HEIGHT_SETTING_ENABLED
#endif  // !CUSTOMIZED
  }

  bool GlobalConst::isPartitionPrintEnabled() const {
#ifndef CUSTOMIZED
    return false;
#else
#  ifdef CUSTOM_PARTITION_PRINT_ENABLED
    return true;
#  else
    return false;
#  endif  // CUSTOM_PARTITION_PRINT_ENABLED
#endif  // !CUSTOMIZED
  }

  QString GlobalConst::getTranslateContext() const {
#ifndef CUSTOMIZED
    return QStringLiteral("global_const");
#else
    return QStringLiteral("global_const_%1").arg(QStringLiteral(CUSTOM_TYPE));
#endif
  }

  int GlobalConst::appType() const {
    return APP_TYPE;
  }

  QString GlobalConst::userCoursePath() {
    QString path;
#if defined(__APPLE__)
    int index = QCoreApplication::applicationDirPath().lastIndexOf("/");
    path = QCoreApplication::applicationDirPath().left(index) + "/Resources/resources/manual/";
#else
    path = QCoreApplication::applicationDirPath() + "/resources/manual/";
#endif
    return path;
  }

  QString GlobalConst::getUiParameterDirPath() {
    QString path;
#ifdef QT_NO_DEBUG
#  if defined(__APPLE__)
    const auto index = QCoreApplication::applicationDirPath().lastIndexOf(QStringLiteral("/"));
    path = QStringLiteral("%1%2").arg(QCoreApplication::applicationDirPath().left(index))
                                 .arg(QStringLiteral("/Resources/resources/sliceconfig/param/ui/"));
#  else
    path = QStringLiteral("%1%2").arg(QCoreApplication::applicationDirPath())
                                 .arg(QStringLiteral("/resources/sliceconfig/param/ui/"));
#  endif
#else
    path = QStringLiteral("%1%2").arg(QStringLiteral(SOURCE_ROOT))
                                 .arg(QStringLiteral("/resources/sliceconfig/param/ui/"));
#endif
    return path;
  }

  QString GlobalConst::userFeedbackWebsite() {
    QString website = "http://as.cxswyjy.com/slice/#/OnlineSupport?";

    QSettings setting;
    setting.beginGroup("cloud_service");
    QString strStartType = setting.value("server_type", "0").toString();
    setting.endGroup();
    QString version = PROJECT_VERSION_EXTRA;
    if (strStartType == "0") {
      if (version == "Release") {
        website += "serverEnv=1";
      } else {
        website += "serverEnv=5";
      }
      website += "&templateId=6638105";
    } else {
      if (version == "Release") {
        website += "serverEnv=2";
      } else {
        website += "serverEnv=4";
      }
      website += "&templateId=6637094";
    }

    setting.beginGroup("language_perfer_config");
    QString strLanguageType = setting.value("language_type", "en.ts").toString();
    setting.endGroup();
    if (strLanguageType == "zh_CN.ts") {
      website += "&language=zh-cn";
    } else {
      website += "&language=en";
    }

    return website;
  }

  QString GlobalConst::calibrationTutorialWebsite() {
    QSettings setting;
    setting.beginGroup("language_perfer_config");
    QString strLanguageType = setting.value("language_type", "en.ts").toString();
    setting.endGroup();
    if (strLanguageType == "en.ts") {
      return "https://wiki.creality.com/en/Software/creality-print/CalibrationTutorial";
    } else {
      return "https://wiki.creality.com/zh/Software/creality-print/CalibrationTutorial";
    }
  }

  QString GlobalConst::officialWebsite() {
    QSettings setting;
    setting.beginGroup("language_perfer_config");
    QString strLanguageType = setting.value("language_type", "en.ts").toString();
    setting.endGroup();
    if (strLanguageType == "zh_CN.ts") {
      return "https://www.creality.cn/about";
    } else if (strLanguageType == "zh_TW.ts") {
      return "https://www.creality.cn/about";
    } else{
        return "https://www.creality.com/about-us";
    }
    return "";
  }

  QString GlobalConst::getResourcePath(ResourcesType resource) {
    QString tails[(int)ResourcesType::re_num_max]{"default",
                                                  "../../cloud_models",
                                                  "Log",
                                                  "Log",
                                                  "Analytics",
                                                  "Slice",
                                                  "GCodes",
                                                  "Profiles",
                                                  "Machines",
                                                  "Materials",
                                                  "Extruders"};

    return qtuser_core::getOrCreateAppDataLocation(tails[(int)resource]);
  }

  QString GlobalConst::languageName(MultiLanguage language) {
    return m_lanNames[(int)language];
  }

  QString GlobalConst::languageTsFile(MultiLanguage language) {
    return m_lanTsFiles[(int)language];
  }

  MultiLanguage GlobalConst::tsFile2Language(const QString& tsFile) {
    MultiLanguage language = MultiLanguage::eLanguage_EN_TS;
    for (int i = 0; i < (int)MultiLanguage::eLanguage_NUM; ++i) {
      if (m_lanTsFiles.at(i) == tsFile) {
        language = (MultiLanguage)i;
        break;
      }
    }

    return language;
  }

  QString GlobalConst::themeName(ThemeCategory theme) {
    switch (theme) {
      case ThemeCategory::tc_light:
        return tr("Light Theme");
      case ThemeCategory::tc_dark:
      default:
        return tr("Dark Theme");
    };
  }

  QString GlobalConst::moneyType() {
    // QSettings setting;
    // setting.beginGroup("language_perfer_config");
    // int nType = setting.value("money_type", "0").toInt();
    //
    // setting.endGroup();
    // return nType;
    //
    // int nType = getMoneyTypeIndex();
    // if (nType == 0)
    //	return "RMB";
    // else if (nType == 1)
    //	return "$";
    // return  "RMB";
    return "";
  }

  QString GlobalConst::cloudTutorialWeb(const QString& name) {
    QSettings setting;
    setting.beginGroup("cloud_service");
    QString strStartType = setting.value("server_type", "-1").toString();

    QString web;
    if (name == "myslices") {
      if (strStartType == "0")
        web = QString("https://www.crealitycloud.cn/post-detail/7776");
      else
        web = QString("https://www.crealitycloud.com/post-detail/9768");
    } else if (name == "mymodels") {
      if (strStartType == "0")
        web = QString("https://www.crealitycloud.cn/post-detail/7703");
      else
        web = QString("https://www.crealitycloud.com/post-detail/9766");
    } else if (name == "myfavorited") {
      if (strStartType == "0")
        web = QString("https://www.crealitycloud.cn/post-detail/7770");
      else
        web = QString("https://www.crealitycloud.com/post-detail/9769");

    } else if (name == "myprinter") {
      web = QString("www.baidu.com");
    }
    return web;
  }

  void GlobalConst::writeRegister(const QString& key, const QVariant& value) {
    qDebug() << QString("GlobalConst::writeRegister [%1]").arg(key);

    QSettings settings;
    QStringList keys = key.split("/");
    int count = keys.count();
    if (count >= 1) {
      QSettings setting;
      for (int i = 0; i < count; ++i) {
        if (i == count - 1) {
          setting.setValue(keys.at(i), value);
        } else {
          setting.beginGroup(keys.at(i));
        }
      }
      for (int i = 0; i < count - 1; ++i) {
        setting.endGroup();
      }
    }
  }

  QVariant GlobalConst::readRegister(const QString& key) {
    qDebug() << QString("GlobalConst::readRegister [%1]").arg(key);

    QVariant result;
    QStringList keys = key.split("/");
    int count = keys.count();
    if (count >= 1) {
      QSettings setting;
      for (int i = 0; i < count; ++i) {
        if (i == count - 1) {
          result = setting.value(keys.at(i));
        } else {
          setting.beginGroup(keys.at(i));
        }
      }
      for (int i = 0; i < count - 1; ++i) {
        setting.endGroup();
      }
    }
    return result;
  }

  void GlobalConst::initialize() {
#ifdef QT_NO_DEBUG
#  ifdef __APPLE__
    int index = QCoreApplication::applicationDirPath().lastIndexOf("/");
    QString src_dir = (QCoreApplication::applicationDirPath().left(index) +
                      "/Resources/resources/sliceconfig/default/");
#  else
    QString src_dir = (QCoreApplication::applicationDirPath() + "/resources/sliceconfig/default/");
#  endif
#else
    QString src_dir = QString(SOURCE_ROOT) + "/resources/sliceconfig/default/";
#endif
    us::SettingDef::instance().initialize(src_dir + "/fdm.def.json");
    cxgcode::SettingDef::instance().initialize(src_dir + "/fdm.def.json");

    QString dst_dir = getResourcePath(ResourcesType::rt_default_config_root);
    QDir config_directory(dst_dir);
    QFile version_file(dst_dir + "/version.txt");

    bool need_copy = true;
    bool old_version_exists = version_file.exists();
    if (config_directory.exists() && old_version_exists) {
      version_file.open(QIODevice::ReadOnly | QIODevice::Text);
      QString local_version = version_file.readLine();
      version_file.close();
      need_copy = version() != local_version;
    }
    bool needCopyOldVersion = false;
    QString oldVersionDir;
    if (!version_file.exists()) {
      QString appDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
      QDir app_dir(appDir);
      int lastVersionMajor = 0;
      int lastVersionMinor = 0;
      for (const auto& entry : app_dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        const auto versionDir = entry.fileName().split(".");
        if (versionDir.size() != 2) {
          continue;
        }
        int tempVersionMajor = versionDir[0].toInt();
        int tempVersionMinor = versionDir[1].toInt();
        if (tempVersionMajor > lastVersionMajor ||
            (tempVersionMajor == lastVersionMajor && tempVersionMinor > lastVersionMinor)) {
          lastVersionMajor = tempVersionMajor;
          lastVersionMinor = tempVersionMinor;
        }
      }
      if (lastVersionMajor != 0 || lastVersionMinor != 0) {
        needCopyOldVersion = true;
        oldVersionDir = QString("%1/%2.%3").arg(appDir).arg(lastVersionMajor).arg(lastVersionMinor);
      }
    }

    if (need_copy) {
      {
        QSettings updaterSettings;
        updaterSettings.setValue(QString::fromLatin1("AutoCheckForUpdates"), true);
      }
      if (config_directory.exists()) {
        config_directory.removeRecursively();
      }

      if (!qtuser_core::copyDir(src_dir, dst_dir, true)) {
        qDebug()
          << QString(
              "initializeConfig failed ! please check access right ! source: [%1], destination:[%2]")
              .arg(src_dir)
              .arg(dst_dir);
        return;
      }
      if (old_version_exists) {
        removeOldDefaultFiles();
      }

#ifndef QT_NO_DEBUG
#  ifdef CUSTOM_MACHINE_LIST
      {  // Debug 模式下定制化设备列表文件位于 customized 目录, 需要单独拷贝
        constexpr auto FILE_NAME{ "machineList_custom.json" };
#    ifdef QT_NO_DEBUG
        const auto IFPATH{ src_dir.toStdString() + FILE_NAME };
#    else
        const auto IFPATH{
          std::string{ SOURCE_ROOT } + "/customized/" + CUSTOM_TYPE + "/" + FILE_NAME };
#    endif
        const auto OFPATH{ dst_dir.toStdString() + "/" + FILE_NAME };
        if (!std::ifstream{ OFPATH }.is_open()) {
          std::ifstream ifstream{ IFPATH };
          std::ofstream ofstream{ OFPATH };
          if (ifstream.is_open() && ofstream.is_open()) {
            char buffer[1024]{ 0 };
            while (ifstream.good()) {
              ifstream.read(buffer, sizeof(buffer));
              ofstream.write(buffer, ifstream.gcount());
            }
          }

          ifstream.close();
          ofstream.close();
        }
      }
#  endif
#endif

      version_file.open(QIODevice::WriteOnly | QIODevice::Truncate);
      version_file.write(version().toLocal8Bit());
      version_file.close();
    }
    if (QString(PROJECT_VERSION_EXTRA) != "Release")
    {
        needCopyOldVersion = false;
    }

    if (needCopyOldVersion) {
      copyFromOldVersion(oldVersionDir);
    }
    loadDefaultKeys();
  }

  void GlobalConst::removeOldDefaultFiles() {
    QString dst_dir = getResourcePath(ResourcesType::rt_default_config_root);

    std::string machineUserFile = getResourcePath(ResourcesType::rt_machine).toStdString() + "/machines_user.json";
    QStringList userMachines;

    rapidjson::Document jsonDoc;
    rapidjson::Value machineArray(rapidjson::kArrayType);
    if (QFile{ machineUserFile.c_str() }.exists()) {
        FILE* file = fopen(machineUserFile.c_str(), "r");

        char read_buffer[10 * 1024] = { 0 };
        rapidjson::FileReadStream reader_stream(file, read_buffer, sizeof(read_buffer));
        jsonDoc.ParseStream(reader_stream);
        fclose(file);
        if (!jsonDoc.HasParseError()) {
            if (jsonDoc.HasMember("machines") && jsonDoc["machines"].IsArray()) {
                const auto& array = jsonDoc["machines"];
                for (size_t i = 0; i < array.Size(); i++) {
                    const auto& machine = array[i];
                    QString machineName;
                    QString machineDiameter;
                    if (machine.HasMember("name") && machine["name"].IsString()) {
                        machineName = machine["name"].GetString();
                    }
                    if (machine.HasMember("supportExtruderDiameters") && machine["supportExtruderDiameters"].IsString()) {
                        machineDiameter = machine["supportExtruderDiameters"].GetString();
                    }
                    userMachines.push_back(
                        QString("%1_%2").arg(machineName).arg(machineDiameter));
                }
            }
        }
    }

    QDir profile_dir(getResourcePath(ResourcesType::rt_profile));
    for (const auto& entry : profile_dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
      if (userMachines.contains(entry.fileName()))
      {
          continue;
      }
      QDir machine_dir(entry.absoluteFilePath());
      for (const auto& profile_entry :
          machine_dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot)) {
        QFile profile(profile_entry.absoluteFilePath());
        const QString& profileFileName = profile_entry.fileName();
        if (profileFileName == "low.default" || profileFileName == "middle.default" ||
            profileFileName == "high.default") {
          profile.remove();
        }
      }
    }

    QDir machine_dir(getResourcePath(ResourcesType::rt_machine));
    for (const auto& entry : machine_dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot)) {
      QFile machine(entry.absoluteFilePath());
      const auto uniqueName = entry.fileName();
      if (!uniqueName.endsWith(".default")) {
        continue;
      }
      int nNozzlePos = uniqueName.lastIndexOf("_");
      QString machineName = uniqueName;
      machineName.truncate(nNozzlePos);
      QFile originMachine(dst_dir + "/Machines/" + machineName + ".default");
      if (originMachine.exists()) {
        machine.remove();
      }
    }

    QDir extruder_dir(getResourcePath(ResourcesType::rt_extruder));
    for (const auto& entry : extruder_dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot)) {
      QFile extruder(entry.absoluteFilePath());
      const auto uniqueName = entry.fileName();
      QRegExp regex("([a-zA-Z0-9-_ ]+)(_\\d.\\d_extruder_\\d.default)");
      if (regex.exactMatch(entry.fileName())) {
        QString machineName = regex.cap(1);
        QFile originMachine(dst_dir + "/Machines/" + machineName + ".default");
        if (originMachine.exists()) {
          extruder.remove();
        }
      }
    }

    QDir material_dir(getResourcePath(ResourcesType::rt_material));
    for (const auto& entry : material_dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        if (userMachines.contains(entry.fileName()))
        {
            continue;
        }
      QStringList userMaterials;
      machine_dir = QDir(entry.absoluteFilePath());

      std::string materialUserFile = entry.absoluteFilePath().toStdString() + "/materials_user.json";

      rapidjson::Document jsonDoc;
      rapidjson::Value machineArray(rapidjson::kArrayType);
      if (QFile{ materialUserFile.c_str() }.exists()) {
        FILE* file = fopen(materialUserFile.c_str(), "r");

        char read_buffer[10 * 1024] = {0};
        rapidjson::FileReadStream reader_stream(file, read_buffer, sizeof(read_buffer));
        jsonDoc.ParseStream(reader_stream);
        fclose(file);
        if (!jsonDoc.HasParseError()) {
          if (jsonDoc.HasMember("materials") && jsonDoc["materials"].IsArray()) {
            const auto& array = jsonDoc["materials"];
            for (size_t i = 0; i < array.Size(); i++) {
              const auto& material = array[i];
              QString materialName;
              QString materialDiameter;
              if (material.HasMember("name") && material["name"].IsString()) {
                materialName = material["name"].GetString();
              }
              if (material.HasMember("supportDiameters") && material["supportDiameters"].IsString()) {
                materialDiameter = material["supportDiameters"].GetString();
              }
              userMaterials.push_back(
                QString("%1_%2.default").arg(materialName).arg(materialDiameter));
            }
          }
        }
      }

      QSettings settings;
      settings.beginGroup("PresetMachines");
      settings.beginGroup(entry.fileName());
      settings.beginGroup("Extruders");
      const auto& groups = settings.childGroups();
      for (int i = 0; i < groups.size(); ++i)
      {
          settings.beginGroup(groups[i]);
          settings.beginGroup("PresetMaterials");
          settings.remove("");
          settings.endGroup();
          settings.endGroup();
      }
      settings.endGroup();
      settings.endGroup();
      settings.endGroup();

      for (const auto& profile_entry :
          machine_dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot)) {
        QFile profile(profile_entry.absoluteFilePath());
        const QString& profileFileName = profile_entry.fileName();
        if (!profileFileName.endsWith(".default"))
        {
            continue;
        }
        if (!userMaterials.contains(profileFileName)) {
          profile.remove();
        }
      }
    }
  }

  void GlobalConst::copyFromOldVersion(const QString& oldVersionDir) {
    QString dst_dir = getResourcePath(ResourcesType::rt_default_config_root);
    QDir profile_dir(oldVersionDir + "/Profiles/");
    for (const auto& entry : profile_dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
      QDir machine_dir(entry.absoluteFilePath());
      for (const auto& profile_entry :
          machine_dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot)) {
        QFile profile(profile_entry.absoluteFilePath());
        const QString& profileFileName = profile_entry.fileName();
        if (profileFileName == "low.default" || profileFileName == "middle.default" ||
            profileFileName == "high.default") {
          continue;
        }
        QString curProfileDir =
          getResourcePath(ResourcesType::rt_profile) + "/" + machine_dir.dirName();
        QString dstFile = curProfileDir + "/" + profileFileName;
        if (!QDir(curProfileDir).exists()) {
          mkMutiDirFromFileName(dstFile);
        }
        profile.copy(dstFile);
      }
    }

    QDir machine_dir(oldVersionDir + "/Machines/");

    for (const auto& entry : machine_dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot)) {
      QFile machine(entry.absoluteFilePath());
      const auto uniqueName = entry.fileName();
      if (!uniqueName.endsWith(".cover")) {
        int nNozzlePos = uniqueName.lastIndexOf("_");
        QString machineName = uniqueName;
        machineName.truncate(nNozzlePos);
        QFile originMachine(dst_dir + "/Machines/" + machineName + ".default");
        if (originMachine.exists()) {
          continue;
        }
      }
      QString dstFile = getResourcePath(ResourcesType::rt_machine) + "/" + uniqueName;
      machine.copy(dstFile);
    }

    QDir extruder_dir(oldVersionDir + "/Extruders/");
    for (const auto& entry : extruder_dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot)) {
      QFile extruder(entry.absoluteFilePath());
      const auto uniqueName = entry.fileName();
      QRegExp regex("([a-zA-Z0-9-_ ]+)(_\\d.\\d_extruder_\\d.default)");
      if (regex.exactMatch(entry.fileName())) {
        QString machineName = regex.cap(1);
        QFile originMachine(dst_dir + "/Machines/" + machineName + ".default");
        if (originMachine.exists()) {
          continue;
        }
      }
      QString dstFile = getResourcePath(ResourcesType::rt_extruder) + "/" + uniqueName;
      extruder.copy(dstFile);
    }

    QDir material_dir(oldVersionDir + "/Materials/");
    for (const auto& entry : material_dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
      QStringList userMaterials;
      machine_dir = QDir(entry.absoluteFilePath());

      std::string materialUserFile = entry.absoluteFilePath().toStdString() + "/materials_user.json";

      rapidjson::Document jsonDoc;
      if (QFile{ materialUserFile.c_str() }.exists()) {
        FILE* file = fopen(materialUserFile.c_str(), "r");

        char read_buffer[10 * 1024] = {0};
        rapidjson::FileReadStream reader_stream(file, read_buffer, sizeof(read_buffer));
        jsonDoc.ParseStream(reader_stream);
        fclose(file);
        if (!jsonDoc.HasParseError()) {
          if (jsonDoc.HasMember("materials") && jsonDoc["materials"].IsArray()) {
            const auto& array = jsonDoc["materials"];
            for (size_t i = 0; i < array.Size(); i++) {
              const auto& material = array[i];
              QString materialName;
              QString materialDiameter;
              if (material.HasMember("name") && material["name"].IsString()) {
                materialName = material["name"].GetString();
              }
              if (material.HasMember("supportDiameters") && material["supportDiameters"].IsString()) {
                materialDiameter = material["supportDiameters"].GetString();
              }
              userMaterials.push_back(
                QString("%1_%2.default").arg(materialName).arg(materialDiameter));
            }
          }
        }
      }

      for (const auto& profile_entry :
          machine_dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot)) {
        QFile profile(profile_entry.absoluteFilePath());
        const QString& profileFileName = profile_entry.fileName();
        if (!userMaterials.contains(profileFileName)) {
          continue;
        }
        QString dstFile = getResourcePath(ResourcesType::rt_extruder) + "/" + profileFileName;
        profile.copy(dstFile);
      }
    }
  }

}  // namespace creative_kernel
