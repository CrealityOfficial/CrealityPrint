#include "kernel/globalconst.h"

#include <fstream>

#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QStandardPaths>

#include <rapidjson/document.h>
#include <rapidjson/filereadstream.h>

#include <buildinfo.h>

#include <cxcloud/define.h>
#include <cxcloud/service_center.h>
#include <cxcloud/tool/function.h>
#include <cxcloud/tool/settings.h>

#include <cxgcode/us/settingdef.h>

#include <qtusercore/module/systemutil.h>
#include <qtusercore/string/resourcesfinder.h>
#include <qtusercore/util/settings.h>

#include "external/interface/cloudinterface.h"
#include "internal/parameter/parameterpath.h"
#include "kernel/kernel.h"

// #pragma execution_character_set("utf-8")

namespace creative_kernel {

  GlobalConst::GlobalConst(QObject* parent)
      : cxkernel::CXKernelConst{ parent }
      , m_lanTsFiles{
          QStringLiteral("en"),
          QStringLiteral("zh_CN"),
          QStringLiteral("zh_TW"),
          QStringLiteral("zh_CN_orca"),
          QStringLiteral("en_c3d"),
          QStringLiteral("zh_CN_c3d"),
          QStringLiteral("zh_TW_c3d"),
        }
      , m_lanNames{
          QStringLiteral("English"),
          QStringLiteral("简体中文"),
          QStringLiteral("繁體中文"),
        } {
    QCoreApplication::setOrganizationName(QStringLiteral(ORGANIZATION));
    QCoreApplication::setOrganizationDomain(QStringLiteral("FDM"));
#ifdef Q_OS_LINUX
    QCoreApplication::setApplicationName(
      QStringLiteral(BUNDLE_NAME).replace(QStringLiteral("_"), QStringLiteral(" ")));
#else
    QCoreApplication::setApplicationName(QStringLiteral("Creative3D"));
#endif

    qtuser_core::VersionSettings setting;
    setting.setValue("engine_type", 1);
    setEngineType(static_cast<EngineType>(setting.value("engine_type", 1).toInt()));
    //initialize();

    QString gcodeDir = qtuser_core::getOrCreateAppDataLocation("GCodes");
    clearPath(gcodeDir);
    QString serialDir = qtuser_core::getOrCreateAppDataLocation("RecentSerializeModel");
    clearPath(serialDir);
  }

  GlobalConst::~GlobalConst() {
      QString serialDir = qtuser_core::getOrCreateAppDataLocation("RecentSerializeModel");
      clearPath(serialDir);
      QString gcodeDir = qtuser_core::getOrCreateAppDataLocation("GCodes");
      clearPath(gcodeDir);
  }
  bool GlobalConst::isAlpha() const{
      return  QStringLiteral(PROJECT_VERSION_EXTRA).toLower() == "alpha";
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
bool GlobalConst::isLaserEnabled() const {
  #ifdef ENABLE_LASER_PLUGIN
    return true;
  #endif
  return false;
}
bool GlobalConst::isDebug() const
{
#ifdef DEBUG
    return true;
#endif // DEBUG
    return false;
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
    path = QStringLiteral("%1%2%3/param/ui/").arg(QCoreApplication::applicationDirPath().left(index))
                                 .arg(QStringLiteral("/Resources/resources/sliceconfig/")).arg(getEnginePathPrefix());
#  else
    path = QStringLiteral("%1%2%3/param/ui/").arg(QCoreApplication::applicationDirPath())
                                 .arg(QStringLiteral("/resources/sliceconfig/")).arg(getEnginePathPrefix());
#  endif
#else
#  if defined(__APPLE__)
    const auto index = QCoreApplication::applicationDirPath().lastIndexOf(QStringLiteral("/"));
    path = QStringLiteral("%1%2%3/param/ui/").arg(QCoreApplication::applicationDirPath().left(index))
                                 .arg(QStringLiteral("/Resources/resources/sliceconfig/")).arg(getEnginePathPrefix());
    qDebug()<<path;
#  else
    path = QStringLiteral("%1%2%3/param/ui/").arg(QStringLiteral(SOURCE_ROOT))
                               .arg(QStringLiteral("/resources/sliceconfig/")).arg(getEnginePathPrefix());
#  endif
#endif

    return path;
  }

  QString GlobalConst::userFeedbackWebsite() {
    QString website = "http://as.cxswyjy.com/slice/#/OnlineSupport?";

    qtuser_core::VersionSettings setting;
    setting.beginGroup("profile_setting");
    QString strStartType = setting.value("server_type", "0").toString();
    setting.endGroup();
    QString version = PROJECT_VERSION_EXTRA;
    if (strStartType == "0") {
      if (version != "Alpha") {
        website += "serverEnv=1";
      } else {
        website += "serverEnv=5";
      }
      website += "&templateId=6638105";
    } else {
      if (version != "Alpha") {
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
    qtuser_core::VersionSettings setting;
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
    qtuser_core::VersionSettings setting;
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

  QString GlobalConst::getEnginePathPrefix()
  {
      return m_enginePathPrefix;
  }

  EngineType GlobalConst::getEngineType() const
  {
      return m_engineType;
  }

  int GlobalConst::getEngineIntType() const
  {
      return (int)m_engineType;
  }

  void GlobalConst::setEngineType(const EngineType& engineType)
  {
      m_engineType = engineType;
      auto server_type = cxcloud::CloudSettings{}.getServerType();
      auto server_index = static_cast<int>(cxcloud::ServerTypeToRealServerType(server_type));

      switch (engineType)
      {
      case EngineType::ET_CURA:
      {
          m_enginePathPrefix = QStringLiteral("server_%1/cura/").arg(QString::number(server_index));
      }
      break;
      case EngineType::ET_ORCA:
      {
          m_enginePathPrefix = QStringLiteral("server_%1/orca/").arg(QString::number(server_index));
      }
      break;
      default:
          break;
      }
  }

  QString GlobalConst::getEngineVersion() const
  {
      return m_engineVersion;
  }

  void GlobalConst::setEngineVersion(const QString& engineVersion)
  {
      if (m_engineVersion == engineVersion)
      {
          return;
      }
      m_engineVersion = engineVersion;
  }

  QString GlobalConst::languageName(MultiLanguage language) {
    return m_lanNames[(int)language];
  }

  QString GlobalConst::languageTsFile(MultiLanguage language) {
    return m_lanTsFiles[(int)language];
  }

  MultiLanguage GlobalConst::tsFile2Language(const QString& tsFile) {
    MultiLanguage language = MultiLanguage::eLanguage_EN_TS;
    QString fileName = tsFile;
    fileName.remove(".ts");
    for (int i = 0; i < (int)MultiLanguage::eLanguage_NUM; ++i) {
      if (m_lanTsFiles.at(i) == fileName) {
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
    // qtuser_core::VersionSettings setting;
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
    auto type = cxcloud::ServerTypeToRealServerType(cxcloud::CloudSettings{}.getServerType());

    QString web;
    if (name == "myslices") {
      if (type == cxcloud::RealServerType::MAINLAND_CHINA)
        web = QString("https://www.crealitycloud.cn/post-detail/7776");
      else
        web = QString("https://www.crealitycloud.com/post-detail/9768");
    } else if (name == "mymodels") {
      if (type == cxcloud::RealServerType::MAINLAND_CHINA)
        web = QString("https://www.crealitycloud.cn/post-detail/7703");
      else
        web = QString("https://www.crealitycloud.com/post-detail/9766");
    } else if (name == "myfavorited") {
      if (type == cxcloud::RealServerType::MAINLAND_CHINA)
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

    qtuser_core::VersionSettings settings;
    QStringList keys = key.split("/");
    int count = keys.count();
    if (count >= 1) {
      qtuser_core::VersionSettings setting;
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
      qtuser_core::VersionSettings setting;
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
#if 1
#  ifdef __APPLE__
    int index = QCoreApplication::applicationDirPath().lastIndexOf("/");
    QString src_dir = QStringLiteral("%1%2%3/default/").arg(QCoreApplication::applicationDirPath().left(index))
        .arg(QStringLiteral("/Resources/resources/sliceconfig/")).arg(getEnginePathPrefix());
#  else
    QString src_dir = QStringLiteral("%1%2%3/default/").arg(QCoreApplication::applicationDirPath())
        .arg(QStringLiteral("/resources/sliceconfig/")).arg(getEnginePathPrefix());
#  endif
#else
    QString src_dir = QString(SOURCE_ROOT) + "/resources/sliceconfig/default/";
#endif
    QString dst_dir = _pfpt_default_root + "/";
    QDir profile_dir(dst_dir);
    //QStringList jsonfiles;
    //for (const auto& entry : profile_dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot)) {
    //    QFile json_file(entry.absoluteFilePath());
    //    if (entry.fileName() == "base.json")
    //    {
    //        continue;
    //    }
    //    jsonfiles.push_back(entry.absoluteFilePath());
    //}
#if 1
    QDir config_directory(dst_dir);
    QFile version_file(dst_dir + "/version.txt");

    QString appDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    auto old_src_dir = QString("%1/%2.%3/orca/").arg(appDir).arg(5).arg(0);
    QFile old_version_file(old_src_dir + "default/version.txt");
    bool need_transfer = old_version_file.exists() && !version_file.exists();

    bool need_copy = true;
    bool old_version_exists = version_file.exists();
    if (config_directory.exists() && old_version_exists) {
      version_file.open(QIODevice::ReadOnly | QIODevice::Text);
      QString local_version = version_file.readLine();
      version_file.close();
      need_copy = version() != local_version;
    }

    if (need_copy) {
      if (config_directory.exists()) {
        config_directory.removeRecursively();
      }

      if (!config_directory.exists()) {
          mkMutiDirFromFileName(dst_dir);
      }

      if (!qtuser_core::copyDir(src_dir, dst_dir, true)) {
        qDebug()
          << QString(
              "initializeConfig failed ! please check access right ! source: [%1], destination:[%2]")
              .arg(src_dir)
              .arg(dst_dir);
        return;
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
    if (need_transfer)
    {
        //拷贝用户目录
        qtuser_core::copyDir(old_src_dir + "user", _pfpt_user_root, true);
        //但5.0用户耗材存储目录不对，也需要拷贝
        QDir old_param_dir(old_src_dir + "default/parampack");
        for (const auto& entry : old_param_dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
            QDir machine_dir(entry.absoluteFilePath());
            auto materials_user = entry.absoluteFilePath() + "/materials_user.json";
            QFile materials_user_file(materials_user);
            if (!materials_user_file.exists())
            {
                continue;
            }
            QString dstFile = _pfpt_user_parampack + "/" + machine_dir.dirName() + "/materials_user.json";
            materials_user_file.copy(dstFile);
        }
        //拷贝注册表
        copyOldVersionSettings();
    }
#endif
    us::SettingDef::instance().initialize("");
    cxgcode::SettingDef::instance().initialize("");
    loadDefaultKeys();
  }

  void GlobalConst::copyOldVersionSettings() {
      QString cur_machine_name;
      std::vector<QColor> extruderColors;
      std::vector<bool> extruderPhysicals;
      QSettings settings;
      settings.beginGroup("5.0");
      settings.beginGroup("PresetMachines");
      int index = settings.value("machine_curren_index", "-1").toInt();
      auto machineUniqueNames = settings.childGroups();
      if (index != -1)
      {
          cur_machine_name = machineUniqueNames[index];
      }
      if (!cur_machine_name.isEmpty())
      {
          settings.beginGroup(cur_machine_name);
          settings.beginGroup("Extruders");
          for (int extruder = 0; extruder < settings.childGroups().size(); ++extruder)
          {
              settings.beginGroup(QString("%1").arg(extruder));
              auto color = settings.value("Color", 0x3DDF56).toUInt();
              extruderColors.push_back(color);
              extruderPhysicals.push_back(settings.value("IsPhysical", 1).toBool());
              settings.endGroup();
          }
          settings.endGroup();
          settings.endGroup();
      }
      settings.endGroup();
      settings.endGroup();

      cxcloud::VersionServerSettings new_settings;
      new_settings.beginGroup("PresetMachines");
      new_settings.setValue("machine_curren_index", index);
      for (int i = 0; i < machineUniqueNames.size(); ++i)
      {
          const auto& machine_name = machineUniqueNames[i];
          new_settings.beginGroup(machine_name);
          new_settings.beginGroup("Extruders");
          for (int j = 0; j < extruderColors.size(); j++)
          {
              new_settings.beginGroup(QString::number(j));
              new_settings.setValue("Color", extruderColors[j].rgba64().toArgb32());
              bool physical = extruderPhysicals[j];
              new_settings.setValue("IsPhysical", physical);
              new_settings.endGroup();
          }
          new_settings.endGroup();
          new_settings.endGroup();
      }
      new_settings.endGroup();
  }

}  // namespace creative_kernel
