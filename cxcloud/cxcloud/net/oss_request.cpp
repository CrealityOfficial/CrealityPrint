#include "cxcloud/net/oss_request.h"

#include <oss_constant.h>
#include <oss_interface.h>

#include <qtusercore/module/systemutil.h>

namespace cxcloud {

  OssRequest::OssRequest(QObject* parent) : HttpRequest(parent), state_(State::INFO) {}

  auto OssRequest::asyncRequest() -> void {
    state_ = OssRequest::State::INFO;
    future_ = std::async(std::launch::async, [this]() {
      syncPost();
    });
  }

  QString OssRequest::getUrlTail() const {
    switch (state_) {
      case State::INFO: {
        return QStringLiteral("/api/cxy/account/v2/getAliyunInfo");
      }
      case State::BUCKET: {
        return QStringLiteral("/api/cxy/v2/common/getOssInfo");
      }
      case State::REAL: {
        return getRealUrlTail();
      }
      default: {
        return {};
      }
    }
  }

  QByteArray OssRequest::getRequestData() const {
    if (state_ == State::REAL) {
      return getRealRequestData();
    }

    return OssRequest::getRealRequestData();
  }

  void OssRequest::callWhenResponsed() {
    handleResponseData();

    if (state_ == State::INFO) {
      const auto result = responseJson()[QStringLiteral("result")];
      const auto aliyun_info = result[QStringLiteral("aliyunInfo")].toObject();
      if (!aliyun_info.empty()) {
        access_key_id_     = aliyun_info[QStringLiteral("accessKeyId")].toString();
        secret_access_key_ = aliyun_info[QStringLiteral("secretAccessKey")].toString();
        session_token_     = aliyun_info[QStringLiteral("sessionToken")].toString();
      }

      state_ = OssRequest::State::BUCKET;
      syncPost();

    } else if (state_ == State::BUCKET) {
      const auto result = responseJson()[QStringLiteral("result")];
      const auto info = result[QStringLiteral("info")].toObject();
      if (!info.empty()) {
        const auto file     = info[QStringLiteral("file")];
        const auto internal = info[QStringLiteral("internal")];

        end_point_            = info[QStringLiteral("endpoint")].toString();
        file_bucket_          = file[QStringLiteral("bucket")].toString();
        file_prefix_path_     = file[QStringLiteral("prefixPath")].toString();
        internal_bucket_      = internal[QStringLiteral("bucket")].toString();
        internal_prefix_path_ = internal[QStringLiteral("prefixPath")].toString();
      }

      if (isReadyToHandleRealTask()) {
        state_ = State::REAL;
        handleRealTask();
      } else {
        callWhenFailed();
        callAfterFailed();
      }

    } else {
      callWhenSuccessed();
      callAfterSuccessed();
    }
  }

  QString OssRequest::getRealUrlTail() const {
    return {};
  }

  QByteArray OssRequest::getRealRequestData() const {
    return QByteArrayLiteral("{}");
  }

  void OssRequest::handleRealTask() {}

  QString OssRequest::getAccessKeyId() const {
    return access_key_id_;
  }

  QString OssRequest::getSecretAccessKey() const {
    return secret_access_key_;
  }

  QString OssRequest::getSessionToken() const {
    return session_token_;
  }

  QString OssRequest::getEndPoint() const {
    return end_point_;
  }

  QString OssRequest::getFileBucket() const {
    return file_bucket_;
  }

  QString OssRequest::getFilePrefixPath() const {
    return file_prefix_path_;
  }

  QString OssRequest::getInternalBucket() const {
    return internal_bucket_;
  }

  QString OssRequest::getInternalPrefixPath() const {
    return internal_prefix_path_;
  }

  bool OssRequest::initOssConfig() const {
    return false;
  }

  bool OssRequest::isReadyToHandleRealTask() const {
    return !access_key_id_.isEmpty() &&
           !secret_access_key_.isEmpty() &&
           !session_token_.isEmpty() &&
           !end_point_.isEmpty() &&
           !file_bucket_.isEmpty() &&
           !file_prefix_path_.isEmpty() &&
           !internal_bucket_.isEmpty() &&
           !internal_prefix_path_.isEmpty();
  }





  UploadFileRequest::UploadFileRequest(const QStringList& file_list,
                                       const QString& suffix,
                                       FileType file_type,
                                       QObject* parent)
      : OssRequest(parent)
      , file_list_(file_list)
      , suffix_(suffix)
      , file_type_(file_type)
      , total_size_(0)
      , uploaded_size_(0)
      , finished_count_(0)
      , successed_count_(0) {}

  QString UploadFileRequest::getSuffix() const {
    return suffix_;
  }

  void UploadFileRequest::setSuffix(const QString& suffix) {
    suffix_ = suffix;
  }

  QStringList UploadFileRequest::getFileList() const {
    return file_list_;
  }

  void UploadFileRequest::setFileList(const QStringList& file_list) {
    file_list_ = file_list;
  }

  QString UploadFileRequest::getOnlineFilePath(const QString& local_path) {
    auto iter = local_online_path_map_.find(local_path);

    if (iter == local_online_path_map_.cend()) {
      QString prefix_path;
      switch (file_type_) {
        case FileType::FILE: {
          prefix_path = getFilePrefixPath();
          break;
        }
        case FileType::INTERNAL:
        default: {
          prefix_path = getFilePrefixPath();
          break;
        }
      }

      auto online_path = QStringLiteral("%1/%2.%3").arg(prefix_path)
                                                   .arg(qtuser_core::calculateFileMD5(local_path))
                                                   .arg(suffix_);

      iter = local_online_path_map_.emplace(std::make_pair(local_path, std::move(online_path)))
                                   .first;
    }

    return iter->second;
  }

  void UploadFileRequest::handleRealTask() {
    finished_count_ = 0;
    successed_count_ = 0;
    total_size_ = 0;
    uploaded_size_ = 0;
    time_ = QTime::currentTime();
    setTotalProgress(100);

    auto result_callback = [this](const oss_wrapper::OSSError& error) mutable {
      ++finished_count_;

      if (error.code_ == "200") {
        ++successed_count_;
      }

      if (finished_count_ >= file_list_.size()) {
        auto successed = finished_count_ == successed_count_;
        QMetaObject::invokeMethod(this, [this, successed]() {
          successed ? callWhenSuccessed() : callWhenFailed();
          successed ? callAfterSuccessed() : callAfterFailed();

          if (isAutoDelete()) {
            deleteLater();
          }
        }, Qt::ConnectionType::QueuedConnection);
      }
    };

    if (!initOssConfig()) {
      QMetaObject::invokeMethod(this, [this]() {
        callWhenFailed();
        callAfterFailed();
      }, Qt::ConnectionType::QueuedConnection);

      return;
    }

    for (const auto& local_file_path : file_list_) {
      auto progress_callback = [this, initialized = false, last_uploaded_size = 0ll]
                               (long long uploaded_size, long long total_size) mutable {
        if (!initialized) {
          initialized = true;
          total_size_ += total_size;
        }

        auto new_uploaded_size = uploaded_size - last_uploaded_size;
        last_uploaded_size = uploaded_size;
        uploaded_size_ += new_uploaded_size;
        // uploaded_size_ += uploaded_size;

        auto progress = static_cast<double>(uploaded_size_) /
                        static_cast<double>(total_size_) *
                        100.0;

        progress = (std::max)(0., (std::min)(progress, 100.));

        std::unique_lock<std::mutex> locker{ mutex_ };
        setCurrentProgress(static_cast<size_t>(progress));
        callWhenProgressUpdated();
      };

      oss_wrapper::OSSInterface::uploadObject(getOnlineFilePath(local_file_path),
                                              local_file_path,
                                              progress_callback,
                                              result_callback);
    }
  }

  void UploadFileRequest::callWhenProgressUpdated() {
    auto current = QTime::currentTime();
    // update ui every 500 msecs
    if (time_.msecsTo(current) <= 500) { return; }
    time_ = std::move(current);
    callAfterProgressUpdated();
  }

  bool UploadFileRequest::initOssConfig() const {
    QString bucket;
    switch (file_type_) {
      case FileType::FILE: {
        bucket = getFileBucket();
        break;
      }
      case FileType::INTERNAL:
      default: {
        bucket = getInternalBucket();
        break;
      }
    }

    return oss_wrapper::OSSInterface::initOSSConfig(getAccessKeyId(),
                                                    getSecretAccessKey(),
                                                    getEndPoint(),
                                                    bucket,
                                                    getSessionToken());
  }

}  // namespace cxcloud
