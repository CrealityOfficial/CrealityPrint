#include "cxcloud/net/common_request.h"

#include <QtCore/QVariant>

namespace cxcloud {

SsoTokenRequest::SsoTokenRequest(QObject* parent) : HttpRequest(parent) {}

QString SsoTokenRequest::getSsoUrl() const {
  return sso_url_;
}

void SsoTokenRequest::callWhenSuccessed() {
  auto token = getResponseJson()[QStringLiteral("result")][QStringLiteral("ssoToken")].toString();
  sso_url_ = QStringLiteral("%1/my-print?pageType=job&slice-token=%2")
               .arg(getUrlHead())
               .arg(std::move(token));
}

QString SsoTokenRequest::getUrlTail() const {
  return QStringLiteral("/api/cxy/account/v2/getSSOToken");
}

LoadCategoryListRequest::LoadCategoryListRequest(Type type, QObject* parent)
    : HttpRequest(parent)
    , type_(type) {}

QByteArray LoadCategoryListRequest::getRequestData() const {
  return QJsonDocument{ QJsonObject{
    { QStringLiteral("type"), static_cast<int>(type_) }
  } }.toJson(QJsonDocument::JsonFormat::Compact);
}

QString LoadCategoryListRequest::getUrlTail() const {
  return QStringLiteral("/api/cxy/v2/common/categoryList");
}

DownloadRequest::DownloadRequest(const QString& src_url, const QString& dst_path, QObject* parent)
    : HttpRequest(parent), src_url_(src_url), dst_path_(dst_path) {}

QString DownloadRequest::getDownloadSrcUrl() const { return src_url_; }

QString DownloadRequest::getDownloadDstPath() const { return dst_path_; }

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

bool CheckNewestVersionRequest::exist() const { return !uid_.isEmpty(); }

QString CheckNewestVersionRequest::uid() const { return uid_; }

QString CheckNewestVersionRequest::name() const { return name_; }

Version CheckNewestVersionRequest::version() const { return version_; }

QDateTime CheckNewestVersionRequest::time() const { return time_; }

QString CheckNewestVersionRequest::description() const { return description_; }

QString CheckNewestVersionRequest::url() const { return url_; }

QByteArray CheckNewestVersionRequest::getRequestData() const {
  return QJsonDocument{ {
    { QStringLiteral("page")    , 1 },
    { QStringLiteral("pageSize"), 999 },
    { QStringLiteral("type")    , [this]() -> int {
      switch (getApplicationType()) {
        case ApplicationType::CREATIVE_PRINT:
          return 7;
        case ApplicationType::HALOT_BOX:
        default:
          return 6;
      }
    }() },
    // { QStringLiteral("keyword") , QString{} },
    { QStringLiteral("palform") , palform_ },
  } }.toJson(QJsonDocument::JsonFormat::Compact);
}

QString CheckNewestVersionRequest::getUrlTail() const {
  return QStringLiteral("/api/cxy/search/softwareSearch");
}

void CheckNewestVersionRequest::callWhenSuccessed() {
  const auto root = getResponseJson();
  if (root.empty()) { return; }

  const auto file_list = root[QStringLiteral("result")][QStringLiteral("list")].toArray();
  if (file_list.empty()) { return; }

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
  if (!found_any) { return; }

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
