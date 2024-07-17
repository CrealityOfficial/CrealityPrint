#include "cxcloud/service/model_service.h"

#include <QtCore/QFileInfo>
#include <QtCore/QDir>

#include <qtusercore/string/resourcesfinder.h>

#include "cxcloud/net/model_request.h"

namespace cxcloud {

  ModelService::ModelService(std::weak_ptr<HttpSession> http_session, QObject* parent)
      : BaseService{ http_session, parent }
      , cache_dir_name_{ QStringLiteral("temp_upload_model") }
      , uploaded_model_group_model_{ std::make_unique<ModelGroupInfoListModel>() }
      , collected_model_group_model_{ std::make_unique<ModelGroupInfoListModel>() }
      , purchased_model_group_model_{ std::make_unique<ModelGroupInfoListModel>() } {}

  auto ModelService::setUserIdGetter(std::function<QString()> getter) -> void {
    userid_getter_ = getter;
  }

  auto ModelService::setSelectedModelCombineSaver(
      std::function<QString(const QString&)> saver) -> void {
    selected_model_combine_saver_ = saver;
  }

  auto ModelService::setSelectedModelUncombineSaver(
      std::function<QStringList(const QString&)> saver) -> void {
    selected_model_uncombine_saver_ = saver;
  }

  auto ModelService::getUploadedModelGroupListModel() const -> QAbstractListModel* {
    return uploaded_model_group_model_.get();
  }

  auto ModelService::clearUploadedModelGroupListModel() const -> void {
    uploaded_model_group_model_->clear();
  }

  auto ModelService::loadUploadedModelGroupList(int page_index, int page_size) -> void {
    auto request = createRequest<LoadUploadedModelGroupListReuquest>(
      static_cast<size_t>(page_index), static_cast<size_t>(page_size));

    request->setSuccessedCallback([this, request]() {
      SyncHttpResponseDataToModel(*request, *uploaded_model_group_model_);
    });

    request->asyncPost();
  }

  auto ModelService::deleteUploadedModelGroup(const QString& group_uid) -> void {
    auto request = createRequest<DeleteModelRequest>(group_uid);

    request->setSuccessedCallback([this, group_uid]() {
      uploaded_model_group_model_->erase(group_uid);
    });

    request->asyncPost();
  }

  auto ModelService::getCollectedModelGroupListModel() const -> QAbstractListModel* {
    return collected_model_group_model_.get();
  }

  auto ModelService::clearCollectedModelGroupListModel() const -> void {
    collected_model_group_model_->clear();
  }

  auto ModelService::loadCollectedModelGroupList(int page_index, int page_size) -> void {
    auto request = createRequest<LoadCollectedModelGroupListRequest>(
      static_cast<size_t>(page_index), static_cast<size_t>(page_size));

    request->setSuccessedCallback([this, request]() {
      SyncHttpResponseDataToModel(*request, *collected_model_group_model_);
    });

    request->asyncPost();
  }

  auto ModelService::onModelGroupUncollected(const QVariant& group_uid) -> void {
    collected_model_group_model_->erase(group_uid.toString());
  }

  auto ModelService::getPurchasedModelGroupListModel() const -> QAbstractListModel* {
    return purchased_model_group_model_.get();
  }

  auto ModelService::clearPurchasedModelGroupListModel() const -> void {
    purchased_model_group_model_->clear();
  }

  auto ModelService::loadPurchasedModelGroupList(int page_index, int page_size) -> void {
    auto request = createRequest<LoadPurchasedModelGroupListReuquest>(
      static_cast<size_t>(page_index), static_cast<size_t>(page_size));

    request->setSuccessedCallback([this, request]() {
      SyncHttpResponseDataToModel(*request, *collected_model_group_model_);
    });

    request->asyncPost();
  }

  auto ModelService::uploadModelGroup(bool is_local_file,
                                      bool is_original,
                                      bool is_share,
                                      bool is_combine,
                                      int color_index,
                                      const QString& category_id,
                                      const QString& group_name,
                                      const QString& group_desc,
                                      const QString& license,
                                      const QStringList& file_list) -> void {
    if (upload_request_) {
      return;
    }

    const auto cache_dir_path = qtuser_core::getOrCreateAppDataLocation(cache_dir_name_);

    auto upload_file_list = QStringList{};
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

    upload_request_ = createRequest<UploadFileRequest>(upload_file_list,
                                                       QStringLiteral("stl"),
                                                       UploadFileRequest::FileType::INTERNAL);

    upload_request_->setProgressUpdatedCallback([this]() {
      Q_EMIT uploadModelGroupProgressUpdated(
        QVariant::fromValue<int>(upload_request_->getCurrentProgress()));
    });

    upload_request_->setSuccessedCallback([this,
                                           is_original,
                                           is_share,
                                           color_index,
                                           category_id,
                                           group_name,
                                           group_desc,
                                           license,
                                           cache_dir_path,
                                           upload_file_list]() {
      if (upload_request_->isCancelDownloadLater()) {
        Q_EMIT uploadModelGroupCanceled();
        return;
      }

      std::list<CreateModelGroupRequest::FileInfo> file_info_list;
      for (const auto& file_path : upload_file_list) {
        QFileInfo file_info{ file_path };
        file_info_list.push_back({
          upload_request_->getOnlineFilePath(file_path),
          file_info.baseName(),
          static_cast<int>(file_info.size()),
        });
      }

      auto create_request = createRequest<CreateModelGroupRequest>(is_original,
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

      create_request->asyncPost();
    });

    upload_request_->setFailedCallback([this, cache_dir_path]() {
      Q_EMIT uploadModelGroupFailed();
      QDir{ cache_dir_path }.removeRecursively();
    });

    upload_request_->asyncRequest();
  }

  auto ModelService::cancelUploadModelGroup() -> void {
    if (!upload_request_) {
      return;
    }

    upload_request_->setCancelDownloadLater(true);
  }

}  // namespace cxcloud
