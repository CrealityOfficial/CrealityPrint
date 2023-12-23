#pragma once

#ifndef CXCLOUD_SERVICE_MODEL_SERVICE_H_
#define CXCLOUD_SERVICE_MODEL_SERVICE_H_

#include <functional>
#include <memory>

#include <QtCore/QStringList>

#include "cxcloud/model/model_info_model.h"
#include "cxcloud/service/base_service.hpp"

namespace cxcloud {

class CXCLOUD_API ModelService : public BaseService {
  Q_OBJECT;

public:
  explicit ModelService(std::weak_ptr<HttpSession> http_session, QObject* parent = nullptr);
  virtual ~ModelService() = default;

public:
  void setUserIdGetter(std::function<QString()> getter);
  void setSelectedModelCombineSaver(std::function<QString(const QString&)> saver);
  void setSelectedModelUncombineSaver(std::function<QStringList(const QString&)> saver);

public:
  QAbstractListModel* getUploadedModelGroupListModel() const;
  Q_PROPERTY(QAbstractListModel* uploadedModelGroupListModel
             READ getUploadedModelGroupListModel CONSTANT);
  Q_INVOKABLE void clearUploadedModelGroupListModel() const;
  Q_INVOKABLE void loadUploadedModelGroupList(int page_index, int page_size);
  Q_SIGNAL void loadUploadedModelGroupListSuccessed(const QVariant& json_string,
                                                    const QVariant& page_index,
                                                    const QVariant& page_size);

  QAbstractListModel* getCollectedModelGroupListModel() const;
  Q_PROPERTY(QAbstractListModel* collectedModelGroupListModel
             READ getCollectedModelGroupListModel CONSTANT);
  Q_INVOKABLE void clearCollectedModelGroupListModel() const;
  Q_INVOKABLE void loadCollectedModelGroupList(int page_index, int page_size);
  Q_SIGNAL void loadCollectedModelGroupListSuccessed(const QVariant& json_string,
                                                     const QVariant& page_index,
                                                     const QVariant& page_size);
  Q_SLOT void onModelGroupUncollected(const QVariant& group_uid);

  QAbstractListModel* getPurchasedModelGroupListModel() const;
  Q_PROPERTY(QAbstractListModel* purchasedModelGroupListModel
             READ getPurchasedModelGroupListModel CONSTANT);
  Q_INVOKABLE void clearPurchasedModelGroupListModel() const;
  Q_INVOKABLE void loadPurchasedModelGroupList(int page_index, int page_size);
  Q_SIGNAL void loadPurchasedModelGroupListSuccessed(const QVariant& json_string,
                                                     const QVariant& page_index,
                                                     const QVariant& page_size);

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
  Q_SIGNAL void uploadModelGroupSuccessed();
  Q_SIGNAL void uploadModelGroupFailed();
  Q_SIGNAL void uploadModelGroupProgressUpdated(const QVariant& progress);

  Q_INVOKABLE void deleteModelGroup(const QString& group_uid);
  Q_SIGNAL void deleteModelGroupSuccessed(const QVariant& group_uid);
  Q_SIGNAL void deleteModelGroupFailed(const QVariant& group_uid);

private:
  const QString cache_dir_name_;

  std::function<QString()> userid_getter_;
  std::function<QString(const QString&)> selected_model_combine_saver_;
  std::function<QStringList(const QString&)> selected_model_uncombine_saver_;

  std::unique_ptr<ModelGroupInfoListModel> uploaded_model_group_model_;
  std::unique_ptr<ModelGroupInfoListModel> collected_model_group_model_;
  std::unique_ptr<ModelGroupInfoListModel> purchased_model_group_model_;
};

}  // namespace cxcloud

#endif  // CXCLOUD_SERVICE_MODEL_SERVICE_H_
