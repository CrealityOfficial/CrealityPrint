#include "cxcloud/service/slice_service.h"

#include <QtCore/QDir>
#include <QtCore/QVariant>

#include <buildinfo.h>

#include <qtusercore/module/quazipfile.h>
#include <qtusercore/string/resourcesfinder.h>

#include "cxcloud/net/slice_request.h"
#include "cxcloud/net/oss_request.h"

namespace cxcloud {

SliceService::SliceService(std::weak_ptr<HttpSession> http_session, QObject* parent)
    : BaseService(http_session, parent)
    , cache_dir_name_(QStringLiteral("temp_upload_slice"))
    , uploaded_slice_list_model_(std::make_unique<SliceListModel>(getVaildSuffixList()))
    , cloud_slice_list_model_(std::make_unique<SliceListModel>(getVaildSuffixList())) {}

void SliceService::setCurrentSliceSaver(std::function<QString(const QString&)> saver) {
  current_slice_saver_ = saver;
}

void SliceService::setOpenFileHandler(std::function<void(const QString&)> handler) {
  open_file_handler_ = handler;
}

bool SliceService::isOnlineSliceFileCompressed() const {
  return online_slice_file_compressed_;
}

void SliceService::setOnlineSliceFileCompressed(bool compressed) {
  online_slice_file_compressed_ = compressed;
}

QStringList SliceService::getVaildSuffixList() const {
  return vaild_suffix_list_;
}

void SliceService::setVaildSuffixList(const QStringList& list) {
  vaild_suffix_list_ = list;
  uploaded_slice_list_model_->setVaildSuffixList(list);
  cloud_slice_list_model_->setVaildSuffixList(list);
  Q_EMIT vaildSuffixListChanged();
}

QAbstractItemModel* SliceService::getUploadedSliceListModel() const {
  return uploaded_slice_list_model_.get();
}

void SliceService::clearUploadedSliceListModel() const {
  uploaded_slice_list_model_->clear();
}

void SliceService::loadUploadedSliceList(int page_index, int page_size) {
  auto* request = createRequest<LoadSliceListRequest>(true, page_index, page_size);

  request->setSuccessedCallback([this, request]() {
    SyncHttpResponseDataToModel(*request, *uploaded_slice_list_model_);
    Q_EMIT loadUploadedSliceListSuccessed(QVariant::fromValue<QString>(request->getResponseData()),
                                          QVariant::fromValue(request->getPageIndex()),
                                          QVariant::fromValue(request->getPageSize()));
  });

  HttpPost(request);
}

QAbstractItemModel* SliceService::getCloudSliceListModel() const {
  return cloud_slice_list_model_.get();
}

void SliceService::clearCloudSliceListModel() const {
  cloud_slice_list_model_->clear();
}

void SliceService::loadCloudSliceList(int page_index, int page_size) {
  auto* request = createRequest<LoadSliceListRequest>(false, page_index, page_size);

  request->setSuccessedCallback([this, request]() {
    SyncHttpResponseDataToModel(*request, *cloud_slice_list_model_);
    Q_EMIT loadUploadedSliceListSuccessed(QVariant::fromValue<QString>(request->getResponseData()),
                                          QVariant::fromValue(request->getPageIndex()),
                                          QVariant::fromValue(request->getPageSize()));
  });

  HttpPost(request);
}

void SliceService::uploadSlice(const QString& slice_name) {
  const auto cache_dir_path = qtuser_core::getOrCreateAppDataLocation(cache_dir_name_);
  const auto cache_dir_cleaner = [cache_dir_path]() { QDir{ cache_dir_path }.removeRecursively(); };

  if (!current_slice_saver_) { return; }
  const auto slice_file_path = current_slice_saver_(cache_dir_path);
  const auto slice_suffix = slice_file_path.split(QStringLiteral(".")).last();

  auto upload_file_path = slice_file_path;
  auto upload_file_suffix = slice_suffix;
  if (isOnlineSliceFileCompressed()) {
    const auto compressed_file_suffix = qtuser_core::CompressedFileFormat();
    const auto compressed_file_path = QStringLiteral("%1/%2.%3.%4")
      .arg(cache_dir_path).arg(slice_name).arg(slice_suffix).arg(compressed_file_suffix);
    if (!qtuser_core::zipLocalFile(slice_file_path, compressed_file_path)) {
      cache_dir_cleaner();
      return;
    }

    const auto online_file_suffix =
      QStringLiteral("%1.%2").arg(slice_suffix).arg(compressed_file_suffix);

    upload_file_path = compressed_file_path;
    upload_file_suffix = online_file_suffix;
  }

  auto* upload_request = createRequest<UploadFileRequest>(
    QStringList{ upload_file_path },
    std::move(upload_file_suffix),
    UploadFileRequest::FileType::FILE);

  upload_request->setProgressUpdatedCallback([this, upload_request]() {
    Q_EMIT uploadSliceProgressUpdated(
      QVariant::fromValue<int>(upload_request->getCurrentProgress()));
  });

  upload_request->setSuccessedCallback([this,
                                        cache_dir_cleaner,
                                        upload_request,
                                        slice_name,
                                        upload_file_path]() {
    cache_dir_cleaner();

    auto* create_request = createRequest<CreateSliceRequest>(
      slice_name, upload_request->getOnlineFilePath(upload_file_path));

    create_request->setSuccessedCallback([this, cache_dir_cleaner]() {
      Q_EMIT uploadSliceSuccessed();
    });

    create_request->setFailedCallback([this, cache_dir_cleaner]() {
      Q_EMIT uploadSliceFailed();
    });

    HttpPost(create_request);
  });

  upload_request->setFailedCallback([this, cache_dir_cleaner]() {
    cache_dir_cleaner();
    Q_EMIT uploadSliceFailed();
  });

  StartOssRequest(upload_request);
}

void SliceService::importSlice(const QString& uid) {
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
    Q_EMIT importSliceFailed(uid);
    return;
  }

  auto suffix = isOnlineSliceFileCompressed()
      ? QStringLiteral("%1.%2").arg(std::move(info.suffix)).arg(qtuser_core::CompressedFileFormat())
      : std::move(info.suffix);

  auto* request = createRequest<DownloadSliceRequest>(
    qtuser_core::getOrCreateAppDataLocation(QStringLiteral("/myGcodes/%1").arg(uid)),
    std::move(info.name),
    std::move(suffix),
    std::move(info.url)
  );

  request->setSuccessedCallback([this, request, uid]() {
    auto file_path = request->getFilePath();
    if (isOnlineSliceFileCompressed()) {
      auto compressed_file_path = file_path;
      file_path = compressed_file_path.left(compressed_file_path.lastIndexOf('.'));
      if (!qtuser_core::unZipLocalFile(compressed_file_path, file_path)) {
        Q_EMIT importSliceFailed(uid);
        return;
      }
    }

    if (!open_file_handler_) {
      Q_EMIT importSliceFailed(uid);
      return;
    }

    open_file_handler_(file_path);
    Q_EMIT importSliceSuccessed(uid);
  });

  HttpDownload(request);
}

void SliceService::deleteSlice(const QString& uid) {
  QPointer<HttpRequest> request{ createRequest<DeleteSliceRequest>(uid) };

  request->setSuccessedCallback([this, uid]() {
    Q_EMIT deleteSliceSuccessed(QVariant::fromValue(uid));
  });

  request->setFailedCallback([this, uid]() {
    Q_EMIT deleteSliceFailed(QVariant::fromValue(uid));
  });

  HttpPost(request);
}

}  // namespace cxcloud
