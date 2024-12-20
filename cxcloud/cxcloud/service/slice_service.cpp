#include "cxcloud/service/slice_service.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QJsonDocument>
#include <QtCore/QVariant>
#include <QtGui/QDesktopServices>

#include <buildinfo.h>

#include <qtusercore/module/quazipfile.h>
#include <qtusercore/string/resourcesfinder.h>

#include "cxcloud/net/common_request.h"
#include "cxcloud/net/slice_request.h"

namespace cxcloud {

  SliceService::SliceService(std::weak_ptr<HttpSession> http_session, QObject* parent)
      : BaseService{ http_session, parent }
      , cache_dir_name_{ QStringLiteral("temp_upload_slice") }
      , uploaded_slice_list_model_{ std::make_unique<SliceListModel>(getVaildSuffixList()) }
      , cloud_slice_list_model_{ std::make_unique<SliceListModel>(getVaildSuffixList()) } {}

  auto SliceService::setCurrentSliceSaver(std::function<QString(const QString&)> saver) -> void {
    current_slice_saver_ = saver;
  }

  auto SliceService::setOpenFileHandler(std::function<void(const QString&)> handler) -> void {
    open_file_handler_ = handler;
  }

  auto SliceService::isOnlineSliceFileCompressed() const -> bool {
    return online_slice_file_compressed_;
  }

  auto SliceService::setOnlineSliceFileCompressed(bool compressed) -> void {
    online_slice_file_compressed_ = compressed;
  }

  auto SliceService::getWebUrlGetter() const -> std::function<QString(void)> {
    return web_url_getter_;
  }

  auto SliceService::setWebUrlGetter(std::function<QString(void)> getter) -> void {
    web_url_getter_ = getter;
  }

  auto SliceService::getVaildSuffixList() const -> QStringList {
    return vaild_suffix_list_;
  }

  auto SliceService::setVaildSuffixList(const QStringList& list) -> void {
    vaild_suffix_list_ = list;
    uploaded_slice_list_model_->setVaildSuffixList(list);
    cloud_slice_list_model_->setVaildSuffixList(list);
    vaildSuffixListChanged();
  }

  auto SliceService::getUploadedSliceListModel() const -> QAbstractItemModel* {
    return uploaded_slice_list_model_.get();
  }

  auto SliceService::clearUploadedSliceListModel() const -> void {
    uploaded_slice_list_model_->clear();
  }

  auto SliceService::loadUploadedSliceList(int page_index, int page_size) -> void {
    if (page_index <= 0) {
      page_index = 1;
    }

    auto request = createRequest<LoadSliceListRequest>(true, page_index, page_size);

    request->setSuccessedCallback([this, request]() {
      SyncHttpResponseDataToModel(*request, *uploaded_slice_list_model_);
    });

    request->asyncPost();
  }

  auto SliceService::deleteUploadedSlice(const QString& uid) -> void {
    auto request = createRequest<DeleteSliceRequest>(uid);

    request->setSuccessedCallback([this, uid]() {
      uploaded_slice_list_model_->erase(uid);
    });

    request->asyncPost();
  }

  auto SliceService::getCloudSliceListModel() const -> QAbstractItemModel* {
    return cloud_slice_list_model_.get();
  }

  auto SliceService::clearCloudSliceListModel() const -> void {
    cloud_slice_list_model_->clear();
  }

  auto SliceService::loadCloudSliceList(int page_index, int page_size) -> void {
    if (page_index <= 0) {
      page_index = 1;
    }

    auto request = createRequest<LoadSliceListRequest>(false, page_index, page_size);

    request->setSuccessedCallback([this, request]() {
      SyncHttpResponseDataToModel(*request, *cloud_slice_list_model_);
    });

    request->asyncPost();
  }

  auto SliceService::deleteCloudSlice(const QString& uid) -> void {
    auto request = createRequest<DeleteSliceRequest>(uid);

    request->setSuccessedCallback([this, uid]() {
      cloud_slice_list_model_->erase(uid);
    });

    request->asyncPost();
  }

  auto SliceService::uploadSlice(const QString& slice_name) -> void {
    if (upload_request_) {
      return;
    }

    const auto cache_dir_path = qtuser_core::getOrCreateAppDataLocation(cache_dir_name_);
    const auto cache_dir_cleaner = [cache_dir_path]() {
      QDir{ cache_dir_path }.removeRecursively();
    };

    if (!current_slice_saver_) {
      return;
    }

    const auto slice_file_path = current_slice_saver_(cache_dir_path);
    const auto slice_suffix = slice_file_path.split(QStringLiteral(".")).last();

    auto upload_file_path = slice_file_path;
    auto upload_file_suffix = slice_suffix;
    if (isOnlineSliceFileCompressed()) {
      const auto compressed_file_suffix = qtuser_core::CompressedFileFormat();
      const auto compressed_file_path = QStringLiteral("%1/%2.%3.%4").arg(cache_dir_path)
                                                                     .arg(slice_name)
                                                                     .arg(slice_suffix)
                                                                     .arg(compressed_file_suffix);

      if (!qtuser_core::zipLocalFile(slice_file_path, compressed_file_path)) {
        cache_dir_cleaner();
        return;
      }

      upload_file_path = compressed_file_path;
      upload_file_suffix = QStringLiteral("%1.%2").arg(slice_suffix).arg(compressed_file_suffix);
    }

    upload_request_ = createRequest<UploadFileRequest>(QStringList{ upload_file_path },
                                                       std::move(upload_file_suffix),
                                                       UploadFileRequest::FileType::FILE);

    upload_request_->setProgressUpdatedCallback([this]() {
      uploadSliceProgressUpdated(
        QVariant::fromValue<int>(upload_request_->getCurrentProgress()));
    });

    upload_request_->setSuccessedCallback([this,
                                           cache_dir_cleaner,
                                           slice_name,
                                           upload_file_path]() {
      if (upload_request_->isCancelDownloadLater()) {
        uploadSliceCanceled();
        return;
      }

      cache_dir_cleaner();

      auto create_request = createRequest<CreateSliceRequest>(
        slice_name, upload_request_->getOnlineFilePath(upload_file_path));

      create_request->setSuccessedCallback([this, cache_dir_cleaner]() {
        uploadSliceSuccessed();
      });

      create_request->setFailedCallback([this, cache_dir_cleaner]() {
        uploadSliceFailed();
      });

      create_request->asyncPost();
    });

    upload_request_->setFailedCallback([this, cache_dir_cleaner]() {
      cache_dir_cleaner();
      uploadSliceFailed();
    });

    upload_request_->asyncRequest();
  }

  auto SliceService::cancelUploadSlice() -> void {
    if (!upload_request_) {
      return;
    }

    upload_request_->setCancelDownloadLater(true);
  }

  auto SliceService::importSlice(const QString& uid) -> void {
    SliceListModel::Data info;
    do {
      if (uploaded_slice_list_model_->find(uid)) {
        info = uploaded_slice_list_model_->load(uid);
        break;
      }

      if (cloud_slice_list_model_->find(uid)) {
        info = cloud_slice_list_model_->load(uid);
        break;
      }
    } while (false);

    if (info.url.isEmpty()) {
      importSliceFailed(uid);
      return;
    }

    auto suffix = isOnlineSliceFileCompressed()
                ? QStringLiteral("%1.%2").arg(std::move(info.suffix))
                                         .arg(qtuser_core::CompressedFileFormat())
                : std::move(info.suffix);

    auto request = createRequest<DownloadSliceRequest>(
      qtuser_core::getOrCreateAppDataLocation(QStringLiteral("/myGcodes/%1").arg(uid)),
      std::move(info.name),
      std::move(suffix),
      std::move(info.url));

    request->setSuccessedCallback([this, request, uid]() {
      auto file_path = request->getFilePath();
      if (isOnlineSliceFileCompressed()) {
        auto compressed_file_path = file_path;
        file_path = compressed_file_path.left(compressed_file_path.lastIndexOf('.'));
        if (!qtuser_core::unZipLocalFile(compressed_file_path, file_path)) {
          importSliceFailed(uid);
          return;
        }
      }

      if (!open_file_handler_) {
        importSliceFailed(uid);
        return;
      }

      open_file_handler_(file_path);
      importSliceSuccessed(uid);
    });

    request->asyncDownload();
  }

  auto SliceService::openPrintSliceWebpage(const QString& uid) -> void {
    if (!web_url_getter_) {
      return;
    }

    auto web_url = web_url_getter_();
    if (web_url.isEmpty()) {
      return;
    }

    auto request = createRequest<SsoTokenRequest>();

    request->setSuccessedCallback([request, uid, web_url = std::move(web_url)]() {
      auto token = request->getSsoToken();
      if (token.isEmpty()) {
        return;
      }

      QDesktopServices::openUrl(QUrl{ QStringLiteral("%1/user/select-device/%2?slice-token=%3")
                                          .arg(std::move(web_url), uid, std::move(token)) });
    });

    request->asyncPost();
  }

}  // namespace cxcloud
