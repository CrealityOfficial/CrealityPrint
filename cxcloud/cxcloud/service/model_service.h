#pragma once

#ifndef CXCLOUD_SERVICE_MODEL_SERVICE_H_
#define CXCLOUD_SERVICE_MODEL_SERVICE_H_

#include <functional>
#include <memory>

#include <QtCore/QStringList>
#include <QtQml/QJSValue>

#include "cxcloud/model/model_info_model.h"
#include "cxcloud/net/oss_request.h"
#include "cxcloud/service/base_service.h"

namespace cxcloud {

  class CXCLOUD_API ModelService : public BaseService {
    Q_OBJECT;

   public:
    explicit ModelService(std::weak_ptr<HttpSession> http_session, QObject* parent = nullptr);
    virtual ~ModelService() = default;

   public:
    auto setUserIdGetter(std::function<QString()> getter) -> void;
    auto setSelectedModelCombineSaver(std::function<QString(const QString&)> saver) -> void;
    auto setSelectedModelUncombineSaver(std::function<QStringList(const QString&)> saver) -> void;

   public:
    /// @return ModelGroupInfoModel*
    /// @note cpp holds the ownership
    auto getFocusedModelGroup() const -> QObject*;
    Q_PROPERTY(QObject* focusedModelGroup READ getFocusedModelGroup CONSTANT);
    Q_INVOKABLE void setFocusedModelGroupUid(const QString& group_uid,
                                             int            page_index,
                                             int            page_size);
    auto onProjectDeleted(const QString& model_group_uid, const QString& project_uid) -> void;

    auto getUploadedModelGroupListModel() const -> QAbstractListModel*;
    Q_PROPERTY(QAbstractListModel* uploadedModelGroupListModel
               READ getUploadedModelGroupListModel CONSTANT);
    Q_INVOKABLE void clearUploadedModelGroupListModel() const;
    Q_INVOKABLE void loadUploadedModelGroupList(int page_index, int page_size);
    Q_INVOKABLE void deleteUploadedModelGroup(const QString& group_uid);

    auto getCollectedModelGroupListModel() const -> QAbstractListModel*;
    Q_PROPERTY(QAbstractListModel* collectedModelGroupListModel
               READ getCollectedModelGroupListModel CONSTANT);
    Q_INVOKABLE void clearCollectedModelGroupListModel() const;
    Q_INVOKABLE void loadCollectedModelGroupList(int page_index, int page_size);
    auto onModelGroupUncollected(const QVariant& group_uid) -> void;

    auto getPurchasedModelGroupListModel() const -> QAbstractListModel*;
    Q_PROPERTY(QAbstractListModel* purchasedModelGroupListModel
               READ getPurchasedModelGroupListModel CONSTANT);
    Q_INVOKABLE void clearPurchasedModelGroupListModel() const;
    Q_INVOKABLE void loadPurchasedModelGroupList(int page_index, int page_size);

    /// @param is_combine vaild when !is_local_file
    /// @param license vaild when is_original
    /// @param file_list vaild when is_local_file
    Q_INVOKABLE void uploadModelGroup(bool is_local_file,
                                      bool is_original,
                                      bool is_share,
                                      bool is_combine,
                                      int color_index,
                                      const QString& category_id,
                                      const QString& group_name,
                                      const QString& group_desc,
                                      const QString& license,
                                      const QStringList& file_list);
    Q_INVOKABLE void cancelUploadModelGroup();
    Q_SIGNAL void uploadModelGroupProgressUpdated(const QVariant& progress);
    Q_SIGNAL void uploadModelGroupSuccessed();
    Q_SIGNAL void uploadModelGroupCanceled();
    Q_SIGNAL void uploadModelGroupFailed();

   public:  // stateless
    Q_INVOKABLE void loadModelGroupInfo(const QString& group_uid, QJSValue callback) const;
    Q_INVOKABLE void loadModelGroupModelList(const QString& group_uid,
                                             QJSValue       group_info,
                                             QJSValue       callback) const;
    Q_INVOKABLE void loadModelGroupProjectList(const QString& group_uid,
                                               QJSValue       group_info,
                                               QJSValue       callback) const;

   private:
    const QString cache_dir_name_{};

    std::function<QString()> userid_getter_{ nullptr };
    std::function<QString(const QString&)> selected_model_combine_saver_{ nullptr };
    std::function<QStringList(const QString&)> selected_model_uncombine_saver_{ nullptr };

    std::unique_ptr<ModelGroupInfoListModel> uploaded_model_group_model_{ nullptr };
    std::unique_ptr<ModelGroupInfoListModel> collected_model_group_model_{ nullptr };
    std::unique_ptr<ModelGroupInfoListModel> purchased_model_group_model_{ nullptr };
    std::unique_ptr<ModelGroupInfoModel> focus_group_{ nullptr };

    QPointer<UploadFileRequest> upload_request_{ nullptr };
  };

}  // namespace cxcloud

#endif  // CXCLOUD_SERVICE_MODEL_SERVICE_H_
