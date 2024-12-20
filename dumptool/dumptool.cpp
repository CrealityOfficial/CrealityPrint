#include "dumptool.h"

#include <fstream>
#include <string>

#include <QtCore/QCoreApplication>
#include <QtCore/QDateTime>
#include <QtCore/QObject>
#include <QtCore/QSettings>
#include <QtCore/QVariant>
#include <QtGui/QOffscreenSurface>
#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLFunctions>

#include <alibabacloud/oss/OssClient.h>

#include <boost/filesystem.hpp>

#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/prettywriter.h>

#include <quazip/JlCompress.h>

#include <cxcloud/define.h>
#include <cxcloud/tool/settings.h>
#include "cxcloud/tool/function.h"

#include <buildinfo.h>

#ifdef Q_OS_WINDOWS

#include <Windows.h>

QString GetOperatingSystemName() {
  auto name = QSysInfo::prettyProductName();

  auto winbrand = ::LoadLibraryW(L"winbrand.dll");
  if (winbrand) {
    auto BrandingFormatString =
        (PWSTR(WINAPI*)(__in PCWSTR))::GetProcAddress(winbrand, "BrandingFormatString");
    if (BrandingFormatString) {
      auto name_data = BrandingFormatString(L"%WINDOWS_LONG%");
      auto name_size = lstrlenW(name_data);
      name = QString::fromWCharArray(name_data, name_size);
    }
  }

  ::FreeLibrary(winbrand);
  winbrand = NULL;
  return name;
}

#else

const auto GetOperatingSystemName = QSysInfo::prettyProductName;

#endif  // Q_OS_WINDOWS

DumpTool::DumpTool(QObject* parent) : QObject(parent) {}

DumpTool::DumpTool(const QString& dump_file_path,
                   const QString& scene_file_path,
                   const QString& user_id,
                   QObject* parent)
    : QObject(parent)

    , dump_file_path_(dump_file_path)
    , dump_file_name_(dump_file_path.split(QStringLiteral("/")).last())
    , dump_dir_path_(dump_file_path.left(dump_file_path.lastIndexOf(QStringLiteral("/"))))
    , dump_file_ready_(false)

    , scene_file_path_(scene_file_path)
    , scene_file_name_(scene_file_path.split(QStringLiteral("/")).last())
    , scene_dir_path_(scene_file_path.left(scene_file_path.lastIndexOf(QStringLiteral("/"))))
    , scene_file_ready_(false)

    , temp_dir_name_(QStringLiteral("%1_%2_%3_%4_log")
      .arg(PROJECT_NAME)
      .arg(QStringLiteral(PROJECT_VERSION).replace(" ", "_").replace(".", "_"))
      .arg(user_id)
      .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMddhhmmss"))))
    , temp_dir_path_(QStringLiteral("%1/%2/").arg(dump_dir_path_).arg(temp_dir_name_))

    , info_json_name_(QStringLiteral("info.json"))
    , info_json_path_(QStringLiteral("%1/%2").arg(temp_dir_path_).arg(info_json_name_))

    , zip_file_name_(QStringLiteral("%1.zip").arg(temp_dir_name_))
    , zip_file_path_(QStringLiteral("%1/%2").arg(dump_dir_path_).arg(zip_file_name_)) {
  QOffscreenSurface surface;
  surface.create();

  QOpenGLContext context;
  context.create();
  context.makeCurrent(&surface);

  GLint major = 0;
  GLint minor = 0;
  context.functions()->glGetIntegerv(GL_MAJOR_VERSION, &major);
  context.functions()->glGetIntegerv(GL_MINOR_VERSION, &minor);

  opengl_version_ = QStringLiteral("%1.%2").arg(QString::number(major)).arg(QString::number(minor));
  opengl_vendor_ = reinterpret_cast<const char*>(context.functions()->glGetString(GL_VENDOR));
  opengl_renderer_ = reinterpret_cast<const char*>(context.functions()->glGetString(GL_RENDERER));
  graphics_card_ = opengl_renderer_.split(QStringLiteral("/")).front();

  context.doneCurrent();
}

DumpTool::~DumpTool() {

}

QString DumpTool::getDumpFilePath() const {
  return dump_file_path_;
}

QString DumpTool::getApplicationName() const {
  // return QCoreApplication::applicationName();
  return QString{ BUNDLE_NAME };
}

QString DumpTool::getApplicationVersion() const {
  return QCoreApplication::applicationVersion();
}

QString DumpTool::getApplicationLanguage() const {
  qtuser_core::VersionSettings setting;
  setting.beginGroup(QStringLiteral("language_perfer_config"));
  QString langauge = setting.value(
    QStringLiteral("language_type"), QStringLiteral("en.ts")).toString();
  setting.endGroup();

  if (QStringLiteral("zh_CN.ts") == langauge) {
    return QStringLiteral("简体中文");
  } else if (QStringLiteral("zh_CN.ts") == langauge) {
    return QStringLiteral("繁體中文");
  } else {
    return QStringLiteral("English");
  }
}

QString DumpTool::getOperatingSystemName() const {
  return GetOperatingSystemName();
}

QString DumpTool::getGraphicsCardName() const {
  return graphics_card_;
}

QString DumpTool::getOpenglVersion() const {
  return opengl_version_;
}

QString DumpTool::getOpenglVendor() const {
  return opengl_vendor_;
}

QString DumpTool::getOpenglRenderer() const {
  return opengl_renderer_;
}

void DumpTool::sendReport() {
  bool successed{ false };

  do {
    if (!boost::filesystem::create_directory(temp_dir_path_.toStdString())) {
      break;
    }

#if DUMPTOOL_OPTION_DUMP_FILE
    dump_file_ready_ = prepareDumpFile();
#endif

#if DUMPTOOL_OPTION_SCENE_FILE
    scene_file_ready_ = prepareSceneFile();
#endif

#if DUMPTOOL_OPTION_INFO_JSON
    if (!prepareInfoJson()) {
      break;
    }
#endif

    if (!JlCompress::compressDir(zip_file_path_, temp_dir_path_)) {
      break;
    }

    AlibabaCloud::OSS::InitializeSdk();

    const bool is_mainland_china =
        cxcloud::ServerTypeToRealServerType(cxcloud::CloudSettings{}.getServerType()) ==
        cxcloud::RealServerType::MAINLAND_CHINA;

    const std::string endpoint{ is_mainland_china ? DUMPTOOL_ENDPOINT : DUMPTOOL_ENDPOINT_FOREIGN };
    static const std::string ACCESS_KEY_ID{ DUMPTOOL_ACCESS_KEY_ID };
    static const std::string ACCESS_KEY_SECRET{ DUMPTOOL_ACCESS_KEY_SECRET };
    AlibabaCloud::OSS::ClientConfiguration config;
    AlibabaCloud::OSS::OssClient client{ endpoint, ACCESS_KEY_ID, ACCESS_KEY_SECRET, config };

    static const std::string bucket{ is_mainland_china ? DUMPTOOL_BUCKET : DUMPTOOL_BUCKET_FOREIGN };
    std::string zip_file_object{ DUMPTOOL_LOG_DIR + zip_file_name_.toStdString() };
    auto outcome = client.PutObject(bucket, zip_file_object, zip_file_path_.toStdString());
    if (!outcome.isSuccess()) {
      auto code = outcome.error().Code();
      auto message = outcome.error().Message();
      auto request_id = outcome.error().RequestId();
      break;
    }

    AlibabaCloud::OSS::ShutdownSdk();

    successed = true;
  } while (false);

  boost::filesystem::remove_all(temp_dir_path_.toStdString());
  boost::filesystem::remove_all(zip_file_path_.toStdString());

  Q_EMIT sendReoprtFinished(QVariant::fromValue(successed));
}

bool DumpTool::prepareDumpFile() const {
  auto old_dump_file_path = dump_file_path_.toStdString();
  auto new_dump_file_path = temp_dir_path_.toStdString() + dump_file_name_.toStdString();

  if (!std::ifstream{ old_dump_file_path, std::ios::in | std::ios::binary }.is_open()) {
    return false;
  }

  boost::filesystem::rename(old_dump_file_path, new_dump_file_path);
  return true;
}

bool DumpTool::prepareSceneFile() const {
  auto old_scene_file_path = scene_file_path_.toStdString();
  auto new_scene_file_path = temp_dir_path_.toStdString() + scene_file_name_.toStdString();

  if (!std::ifstream{ old_scene_file_path, std::ios::in | std::ios::binary }.is_open()) {
    return false;
  }

  boost::filesystem::rename(old_scene_file_path, new_scene_file_path);
  return true;
}

bool DumpTool::prepareInfoJson() const {
  rapidjson::Document document;
  document.SetObject();

  auto& allocator = document.GetAllocator();

  rapidjson::Value graphics_card;
  graphics_card.SetString(graphics_card_.toUtf8(), allocator);
  document.AddMember("graphics_card", std::move(graphics_card), allocator);

  rapidjson::Value opengl_version;
  opengl_version.SetString(opengl_version_.toUtf8(), allocator);
  document.AddMember("opengl_version", std::move(opengl_version), allocator);

  rapidjson::Value opengl_vendor;
  opengl_vendor.SetString(opengl_vendor_.toUtf8(), allocator);
  document.AddMember("opengl_vendor", std::move(opengl_vendor), allocator);

  rapidjson::Value opengl_renderer;
  opengl_renderer.SetString(opengl_renderer_.toUtf8(), allocator);
  document.AddMember("opengl_renderer", std::move(opengl_renderer), allocator);

#if DUMPTOOL_OPTION_DUMP_FILE
  if (dump_file_ready_) {
    rapidjson::Value dump_file_name;
    dump_file_name.SetString(dump_file_name_.toUtf8(), allocator);
    document.AddMember("dump_file_name", std::move(dump_file_name), allocator);
  }
#endif

#if DUMPTOOL_OPTION_SCENE_FILE
  if (scene_file_ready_) {
    rapidjson::Value scene_file_name;
    scene_file_name.SetString(scene_file_name_.toUtf8(), allocator);
    document.AddMember("scene_file_name", std::move(scene_file_name), allocator);
  }
#endif

  std::ofstream ofstream{ info_json_path_.toStdString(), std::ios::out | std::ios::binary };
  rapidjson::OStreamWrapper wrapper{ ofstream };
  rapidjson::PrettyWriter<rapidjson::OStreamWrapper> writer{ wrapper };

// #ifdef QT_DEBUG
  writer.SetIndent(' ', 2);
// #endif // QT_DEBUG

  return document.Accept(writer);
}
