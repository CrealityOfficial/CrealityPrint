#include "cxcloud/net/common_request.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>

namespace cxcloud {

  SsoTokenRequest::SsoTokenRequest(QObject* parent) : HttpRequest(parent) {}

  auto SsoTokenRequest::getSsoToken() const -> QString {
    return token_;
  }

  auto SsoTokenRequest::callWhenSuccessed() -> void {
    token_ = getResponseJson()[QStringLiteral("result")][QStringLiteral("ssoToken")].toString();
  }

  auto SsoTokenRequest::getUrlTail() const -> QString {
    return QStringLiteral("/api/cxy/account/v2/getSSOToken");
  }





  LoadCategoryListRequest::LoadCategoryListRequest(Type type, QObject* parent)
      : HttpRequest(parent)
      , type_(type) {}

  auto LoadCategoryListRequest::getRequestData() const -> QByteArray {
    return QJsonDocument{ QJsonObject{
      { QStringLiteral("type"), static_cast<int>(type_) }
    } }.toJson(QJsonDocument::JsonFormat::Compact);
  }

  auto LoadCategoryListRequest::getUrlTail() const -> QString {
    return QStringLiteral("/api/cxy/v2/common/categoryList");
  }





  DownloadRequest::DownloadRequest(const QString& src_url, const QString& dst_path, QObject* parent)
      : HttpRequest(parent), src_url_(src_url), dst_path_(dst_path) {}

  auto DownloadRequest::getDownloadSrcUrl() const -> QString { return src_url_; }

  auto DownloadRequest::getDownloadDstPath() const -> QString { return dst_path_; }





  CheckNewestVersionRequest::CheckNewestVersionRequest(QObject* parent)
      : HttpRequest(parent)
#if defined(Q_OS_WIN)
      , palform_(1)
#elif defined(Q_OS_LINUX)
      , palform_(2)
#elif defined(Q_OS_MACOS)
      , palform_(3)
#endif
      {}

  auto CheckNewestVersionRequest::exist() const -> bool { return !uid_.isEmpty(); }

  auto CheckNewestVersionRequest::uid() const -> QString { return uid_; }

  auto CheckNewestVersionRequest::name() const -> QString { return name_; }

  auto CheckNewestVersionRequest::version() const -> Version { return version_; }

  auto CheckNewestVersionRequest::time() const -> QDateTime { return time_; }

  auto CheckNewestVersionRequest::description() const -> QString { return description_; }

  auto CheckNewestVersionRequest::url() const -> QString { return url_; }

  auto CheckNewestVersionRequest::getRequestData() const -> QByteArray {
    return QJsonDocument{ {
      { QStringLiteral("page")    , 1 },
      { QStringLiteral("pageSize"), 999 },
      { QStringLiteral("type")    , [this]() -> int {
        switch (getApplicationType()) {
          case ApplicationType::CREALITY_PRINT: {
            return 7;
          }
          case ApplicationType::HALOT_BOX:
          default: {
            return 6;
          }
        }
      }() },
      // { QStringLiteral("keyword") , QString{} },
      { QStringLiteral("palform") , palform_ },
    } }.toJson(QJsonDocument::JsonFormat::Compact);
  }

  auto CheckNewestVersionRequest::getUrlTail() const -> QString {
    return QStringLiteral("/api/cxy/search/softwareSearch");
  }

  auto CheckNewestVersionRequest::callWhenSuccessed() -> void {
    const auto root = getResponseJson();
    if (root.empty()) {
      return;
    }

    const auto file_list = root[QStringLiteral("result")][QStringLiteral("list")].toArray();
    if (file_list.empty()) {
      return;
    }

    bool found_any = false;
    QJsonValue newest_file;
    Version newest_version;
    for (const auto& file : file_list) {
      if (file[QStringLiteral("platform")].toInt() != palform_) { continue; }
      found_any = true;

      Version version = file[QStringLiteral("versionNumber")].toString();
      if (newest_version < version) {
        newest_version = std::move(version);
        newest_file = file;
      }
    }
    if (!found_any) {
      return;
    }

    uid_ = newest_file[QStringLiteral("id")].toString();
    name_ = newest_file[QStringLiteral("name")].toString();
    url_ = newest_file[QStringLiteral("fileUrl")].toString();
    version_ = std::move(newest_version);
    time_ = QDateTime::fromSecsSinceEpoch(newest_file[QStringLiteral("createTime")].toInt());
    const auto description_list = newest_file[QStringLiteral("description")].toArray();
    for (const auto& description : description_list) {
      description_ += description.toString();
    }
  }

}  // namespace cxcloud
