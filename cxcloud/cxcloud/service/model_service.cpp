#include "cxcloud/service/model_service.h"

#include <QtCore/QFileInfo>
#include <QtCore/QDir>

#include <qtusercore/string/resourcesfinder.h>

#include "cxcloud/net/model_request.h"
#include "cxcloud/net/oss_request.h"

namespace cxcloud {

ModelService::ModelService(std::weak_ptr<HttpSession> http_session, QObject* parent)
    : BaseService(http_session, parent)
    , cache_dir_name_(QStringLiteral("temp_upload_model"))
    , uploaded_model_group_model_(std::make_unique<ModelGroupInfoListModel>())
    , collected_model_group_model_(std::make_unique<ModelGroupInfoListModel>())
    , purchased_model_group_model_(std::make_unique<ModelGroupInfoListModel>()) {}

void ModelService::setUserIdGetter(std::function<QString()> getter) {
  userid_getter_ = getter;
}

void ModelService::setSelectedModelCombineSaver(std::function<QString(const QString&)> saver) {
  selected_model_combine_saver_ = saver;
}

void ModelService::setSelectedModelUncombineSaver(std::function<QStringList(const QString&)> saver) {
  selected_model_uncombine_saver_ = saver;
}

QAbstractListModel* ModelService::getUploadedModelGroupListModel() const {
  return uploaded_model_group_model_.get();
}

void ModelService::clearUploadedModelGroupListModel() const {
  uploaded_model_group_model_->clear();
}

void ModelService::loadUploadedModelGroupList(int page_index, int page_size) {
  auto* request = createRequest<LoadUploadedModelGroupListReuquest>(
    static_cast<size_t>(page_index), static_cast<size_t>(page_size));

  request->setSuccessedCallback([this, request]() {
    SyncHttpResponseDataToModel(*request, *uploaded_model_group_model_);

    Q_EMIT loadUploadedModelGroupListSuccessed(
      QVariant::fromValue<QString>(request->getResponseData()),
      QVariant::fromValue<int>(request->getPageIndex()),
      QVariant::fromValue<int>(request->getPageSize()));
  });

  HttpPost(request);
}

QAbstractListModel* ModelService::getCollectedModelGroupListModel() const {
  return collected_model_group_model_.get();
}

void ModelService::clearCollectedModelGroupListModel() const {
  collected_model_group_model_->clear();
}

void ModelService::loadCollectedModelGroupList(int page_index, int page_size) {
  auto* request = createRequest<LoadCollectedModelGroupListRequest>(
    static_cast<size_t>(page_index), static_cast<size_t>(page_size));

  request->setSuccessedCallback([this, request]() {
    SyncHttpResponseDataToModel(*request, *collected_model_group_model_);

    Q_EMIT loadCollectedModelGroupListSuccessed(
      QVariant::fromValue<QString>(request->getResponseData()),
      QVariant::fromValue<int>(request->getPageIndex()),
      QVariant::fromValue<int>(request->getPageSize()));
  });

  HttpPost(request);
}

void ModelService::onModelGroupUncollected(const QVariant& group_uid) {
  collected_model_group_model_->erase(group_uid.toString());
}

QAbstractListModel* ModelService::getPurchasedModelGroupListModel() const {
  return purchased_model_group_model_.get();
}

void ModelService::clearPurchasedModelGroupListModel() const {
  purchased_model_group_model_->clear();
}

void ModelService::loadPurchasedModelGroupList(int page_index, int page_size) {
  auto* request = createRequest<LoadPurchasedModelGroupListReuquest>(
    static_cast<size_t>(page_index), static_cast<size_t>(page_size));

  request->setSuccessedCallback([this, request]() {
    SyncHttpResponseDataToModel(*request, *collected_model_group_model_);

    Q_EMIT loadPurchasedModelGroupListSuccessed(
      QVariant::fromValue<QString>(request->getResponseData()),
      QVariant::fromValue<int>(request->getPageIndex()),
      QVariant::fromValue<int>(request->getPageSize()));
  });

  HttpPost(request);
}

void ModelService::uploadModelGroup(bool is_local_file,
                                    bool is_original,
                                    bool is_share,
                                    bool is_combine,
                                    int color_index,
                                    const QString& category_id,
                                    const QString& group_name,
                                    const QString& group_desc,
                                    const QString& license,
                                    const QStringList& file_list) {
  const auto cache_dir_path = qtuser_core::getOrCreateAppDataLocation(cache_dir_name_);

  QStringList upload_file_list;
  if (is_local_file) {
    upload_file_list = file_list;
  } else {
    if (is_combine) {
      if (selected_model_combine_saver_) {
        upload_file_list << selected_model_combine_saver_(cache_dir_path);
      }
    } else {
      if (selected_model_uncombine_saver_) {
        upload_file_list = selected_model_uncombine_saver_(cache_dir_path);
      }
    }
  }

  if (upload_file_list.empty()) {
    return;
  }

  auto* upload_request = createRequest<UploadFileRequest>(
    upload_file_list, QStringLiteral("stl"), UploadFileRequest::FileType::INTERNAL);

  upload_request->setProgressUpdatedCallback([this, upload_request]() {
    Q_EMIT uploadModelGroupProgressUpdated(
      QVariant::fromValue<int>(upload_request->getCurrentProgress()));
  });

  upload_request->setSuccessedCallback([=]() {
    std::list<CreateModelGroupRequest::FileInfo> file_info_list;
    for (const auto& file_path : upload_file_list) {
      QFileInfo file_info{ file_path };
      file_info_list.push_back({
        upload_request->getOnlineFilePath(file_path),
        file_info.baseName(),
        static_cast<int>(file_info.size()),
      });
    }

    auto* create_request = createRequest<CreateModelGroupRequest>(is_original,
                                                                  is_share,
                                                                  color_index,
                                                                  category_id,
                                                                  group_name,
                                                                  group_desc,
                                                                  license,
                                                                  std::move(file_info_list));
    create_request->setSuccessedCallback([this, cache_dir_path]() {
      Q_EMIT uploadModelGroupSuccessed();
      QDir{ cache_dir_path }.removeRecursively();
    });

    create_request->setFailedCallback([this, cache_dir_path]() {
      Q_EMIT uploadModelGroupFailed();
      QDir{ cache_dir_path }.removeRecursively();
    });

    HttpPost(create_request);
  });

  upload_request->setFailedCallback([this, cache_dir_path]() {
    Q_EMIT uploadModelGroupFailed();
    QDir{ cache_dir_path }.removeRecursively();
  });

  StartOssRequest(upload_request);
}

void ModelService::deleteModelGroup(const QString& group_uid) {
  QPointer<HttpRequest> request{ createRequest<DeleteModelRequest>(group_uid) };

  request->setSuccessedCallback([this, group_uid]() {
    uploaded_model_group_model_->erase(group_uid);
    Q_EMIT deleteModelGroupSuccessed(group_uid);
  });

  request->setFailedCallback([this, group_uid]() {
    Q_EMIT deleteModelGroupFailed(group_uid);
  });

  HttpPost(request);
}

}  // namespace cxcloud
