#include "cxcloud/net/download_request.h"

#include <QtCore/QCryptographicHash>
#include <QtCore/QJsonDocument>

namespace cxcloud {

  DownloadModelRequest::DownloadModelRequest(const QString& group_id,
                                             const QString& model_id,
                                             const QString& download_path,
                                             std::weak_ptr<ThreadPool> thread_pool,
                                             QObject* parent)
      : HttpRequest(parent)
      , group_id_(group_id)
      , model_id_(model_id)
      , download_path_(download_path)
      , time_(QTime::currentTime())
      , thread_pool_weak_(thread_pool) {}

  bool DownloadModelRequest::finished() const {
    return finished_;
  }

  bool DownloadModelRequest::successed() const {
    return successed_;
  }

  void DownloadModelRequest::callWhenSuccessed() {
    if (download_url_.isEmpty()) {
      download_url_ = getResponseJson().value(QStringLiteral("result"))
                                       .toObject()
                                       .value(QStringLiteral("signUrl"))
                                       .toString();

      if (!thread_pool_weak_.expired()) {
        thread_pool_shared_ = thread_pool_weak_.lock();
        asyncDownload(*thread_pool_shared_);
      } else {
        asyncDownload();
      }

      return;
    }

    thread_pool_shared_ = nullptr;
    finished_ = true;
    successed_ = true;
    progress_ = 100;
    speed_ = 0;
    Q_EMIT progressUpdated(group_id_, model_id_);
    Q_EMIT downloadFinished(group_id_, model_id_);
  }

  void DownloadModelRequest::callWhenFailed() {
    if (download_url_.isEmpty()) {
      return;
    }

    finished_ = true;
    progress_ = 0;
    speed_ = 0;
    Q_EMIT progressUpdated(group_id_, model_id_);
    Q_EMIT downloadFinished(group_id_, model_id_);
  }

  bool DownloadModelRequest::isAutoDelete() const {
    return false;
  }

  QByteArray DownloadModelRequest::getRequestData() const {
    return QJsonDocument{ QJsonObject{
      { QStringLiteral("id"), model_id_ },
    } }.toJson(QJsonDocument::JsonFormat::Compact);
  }

  QString DownloadModelRequest::getUrlTail() const {
    return QStringLiteral("/api/cxy/v3/model/fileDownload");
  }

  QString DownloadModelRequest::getDownloadDstPath() const {
    return download_path_;
  }

  QString DownloadModelRequest::getDownloadSrcUrl() const {
    return download_url_;
  }

  void DownloadModelRequest::callWhenProgressUpdated() {
    auto current = QTime::currentTime();
    auto duration = time_.msecsTo(current);

    auto total = getTotalProgress();
    auto downloaded = getCurrentProgress();

    // update ui every 500 msecs
    if (duration <= 500) { return; }

    double progress = static_cast<double>(downloaded) / static_cast<double>(total) * 100.;
    progress_ = (std::max)(0., (std::min)(progress, 100.));

    speed_ = (downloaded - downloaded_) / duration;
    time_ = std::move(current);
    downloaded_ = std::move(downloaded);

    Q_EMIT progressUpdated(group_id_, model_id_);
  }

  bool SyncHttpResponseDataToModel(const DownloadModelRequest& request,
                                   DownloadItemListModel& model) {
    auto info = model.load(request.model_id_);

    if (request.finished_) {
      info.state = request.isCancelDownloadLater() ? DownloadItemInfo::State::UNREADY
                 : request.successed()             ? DownloadItemInfo::State::FINISHED
                                                   : DownloadItemInfo::State::FAILED;
      info.date = QDate::currentDate().toString(QStringLiteral("yyyy-MM-dd"));
    } else {
      info.state = DownloadItemInfo::State::DOWNLOADING;
    }

    info.progress = request.progress_;
    info.speed = request.speed_;

    return model.update(std::move(info));
  }

  QByteArray UnlimitedDownloadModelRequest::getRequestData() const {
    const auto header = getHeader();

    const auto sign = QStringLiteral("%1%2%3")
                        .arg(header.at(QStringLiteral("__CXY_PLATFORM_")))
                        .arg(header.at(QStringLiteral("__CXY_REQUESTID_")))
                        .arg(QStringLiteral("CdbbRe3eE79cAf0dL5b1Icb7T799Y218223be01c"));

    const auto encoded_sign =
      QCryptographicHash::hash(sign.toUtf8(), QCryptographicHash::Sha256).toHex();

    return QJsonDocument{ QJsonObject{
      { QStringLiteral("id"), model_id_ },
      { QStringLiteral("sign"), QString{ encoded_sign } },
    } }.toJson(QJsonDocument::JsonFormat::Compact);
  }

  QString UnlimitedDownloadModelRequest::getUrlTail() const {
    return QStringLiteral("/api/cxy/v3/model/downloadUnlimited");
  }

}  // namespace cxcloud
