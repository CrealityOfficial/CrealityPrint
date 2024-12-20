#include "cxcloud/net/common_request.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>

namespace cxcloud {

  SsoTokenRequest::SsoTokenRequest(QObject* parent) : HttpRequest{ parent } {}

  auto SsoTokenRequest::getSsoToken() const -> QString {
    return token_;
  }

  auto SsoTokenRequest::callWhenSuccessed() -> void {
    token_ = responseJson()[QStringLiteral("result")][QStringLiteral("ssoToken")].toString();
  }

  auto SsoTokenRequest::getUrlTail() const -> QString {
    return QStringLiteral("/api/cxy/account/v2/getSSOToken");
  }





  LoadCategoryListRequest::LoadCategoryListRequest(Type type, QObject* parent)
      : HttpRequest{ parent } , type_{ type } {}

  auto LoadCategoryListRequest::getRequestData() const -> QByteArray {
    return QJsonDocument{ QJsonObject{
      { QStringLiteral("type"), static_cast<int>(type_) },
    } }.toJson(QJsonDocument::JsonFormat::Compact);
  }

  auto LoadCategoryListRequest::getUrlTail() const -> QString {
    return QStringLiteral("/api/cxy/v2/common/categoryList");
  }





  DownloadRequest::DownloadRequest(const QString& src_url, const QString& dst_path, QObject* parent)
      : HttpRequest{ parent }, src_url_{ src_url }, dst_path_{ dst_path } {}

  DownloadRequest::~DownloadRequest() {
    if (download_job_) {
      QMetaObject::invokeMethod(download_job_.data(), [download_job = download_job_] {
        download_job->deleteLater();
      }, Qt::ConnectionType::QueuedConnection);
    }
  }

  auto DownloadRequest::getDownloadSrcUrl() const -> QString {
    return src_url_;
  }

  auto DownloadRequest::getDownloadDstPath() const -> QString {
    return dst_path_;
  }

  auto DownloadRequest::getServerFileName() const -> QString {
    return server_file_name_;
  }

  auto DownloadRequest::getJob() const -> QPointer<Job> {
    if (!download_job_) {
      download_job_ = new Job;
      connect(download_job_.data(), &Job::interrupted, this, &DownloadRequest::onJobInterrupted);
    }

    return download_job_;
  }

  auto DownloadRequest::checkRequestData() const -> bool {
    if (download_job_) {
      download_job_->started();
    }

    return HttpRequest::checkRequestData();
  }

  auto DownloadRequest::callWhenProgressUpdated() -> void {
    if (download_job_) {
      auto total_progress = static_cast<float>(getTotalProgress());
      auto current_progress = static_cast<float>(getCurrentProgress());
      download_job_->progressUpdated(current_progress / total_progress);
    }
  }

  auto DownloadRequest::handleResponseData() -> bool {
    if (download_job_) {
      download_job_->finished();
    }

    return HttpRequest::handleResponseData();
  }

  auto DownloadRequest::callWhenSuccessed() -> void {
    const auto root = getResponseJson();
    if (root.empty()) {
      return;
    }

    server_file_name_ = root[QStringLiteral("filename")].toString();
  }

  auto DownloadRequest::onJobInterrupted() -> void {
    setCancelDownloadLater(true);
  }





  CheckNewestVersionRequest::CheckNewestVersionRequest(QObject* parent)
      : HttpRequest{ parent }
#if defined(Q_OS_WIN)
      , palform_{ 1 }
#elif defined(Q_OS_LINUX)
      , palform_{ 2 }
#elif defined(Q_OS_MACOS)
      , palform_{ 3 }
#endif
  {
  }

  auto CheckNewestVersionRequest::exist() const -> bool {
    return !uid_.isEmpty();
  }

  auto CheckNewestVersionRequest::uid() const -> QString {
    return uid_;
  }

  auto CheckNewestVersionRequest::name() const -> QString {
    return name_;
  }

  auto CheckNewestVersionRequest::version() const -> Version {
    return version_;
  }

  auto CheckNewestVersionRequest::time() const -> QDateTime {
    return time_;
  }

  auto CheckNewestVersionRequest::description() const -> QString {
    return description_;
  }

  auto CheckNewestVersionRequest::url() const -> QString {
    return url_;
  }

  auto CheckNewestVersionRequest::getRequestData() const -> QByteArray {
    return QJsonDocument{ {
      { QStringLiteral("page"), 1 },
      { QStringLiteral("pageSize"), 999 },
      { QStringLiteral("type"),
        [this] {
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
      // { QStringLiteral("keyword"), QString{} },
      { QStringLiteral("palform"), palform_ },
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

    auto found_any      = false;
    auto newest_file    = QJsonValue{};
    auto newest_version = Version{};
    for (const auto& file : file_list) {
      if (file[QStringLiteral("platform")].toInt() != palform_) {
        continue;
      }

      found_any = true;

      auto version = Version{ file[QStringLiteral("versionNumber")].toString() };
      if (newest_version < version) {
        newest_version = std::move(version);
        newest_file    = file;
      }
    }

    if (!found_any) {
      return;
    }

    uid_     = newest_file[QStringLiteral("id")].toString();
    name_    = newest_file[QStringLiteral("name")].toString();
    url_     = newest_file[QStringLiteral("fileUrl")].toString();
    version_ = std::move(newest_version);
    time_    = QDateTime::fromSecsSinceEpoch(newest_file[QStringLiteral("createTime")].toInt());
    const auto description_list = newest_file[QStringLiteral("description")].toArray();
    for (const auto& description : description_list) {
      description_ += description.toString();
    }
  }

}  // namespace cxcloud
