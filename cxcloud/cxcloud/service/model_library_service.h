#pragma once

#ifndef CXCLOUD_SERVICE_MODEL_LIBRARY_SERVICE_H_
#define CXCLOUD_SERVICE_MODEL_LIBRARY_SERVICE_H_

#include <functional>
#include <memory>
#include <vector>

#include "cxcloud/model/model_info_model.h"
#include "cxcloud/service/base_service.hpp"

namespace cxcloud {

class CXCLOUD_API ModelLibraryService : public BaseService {
  Q_OBJECT;

public:
  explicit ModelLibraryService(std::weak_ptr<HttpSession> http_session, QObject* parent = nullptr);
  virtual ~ModelLibraryService() = default;

public:
  void initialize() override;
  void uninitialize() override;

  void setModelGroupUrlCreater(std::function<QString(const QString&)> creater);

  QString getCacheDirPath() const;
  void setCacheDirPath(const QString& path);

  QString getHistoryFilePath() const;

public:  // about model group list for every group category
  QAbstractListModel* getModelGroupCategoryModelList();
  Q_PROPERTY(QAbstractListModel* modelGroupCategoryModelList
             READ getModelGroupCategoryModelList CONSTANT);
  Q_INVOKABLE void clearModelGroupCategoryList();
  Q_INVOKABLE void loadModelGroupCategoryList();
  Q_SIGNAL void loadModelGroupCategoryListSuccessed(const QVariant& json_string);

  QAbstractListModel* getSearchModelGroupListModel() const;
  Q_PROPERTY(QAbstractListModel* searchModelGroupListModel
             READ getSearchModelGroupListModel CONSTANT);
  Q_INVOKABLE void clearSearchModelGroupList();
  Q_INVOKABLE void searchModelGroup(const QString& keyword, int page, int size);
  Q_SIGNAL void searchModelGroupSuccessed(const QVariant& json_string, const QVariant& page_index);

  QAbstractListModel* getHistoryModelGroupListModel() const;
  Q_PROPERTY(QAbstractListModel* historyModelGroupListModel
             READ getHistoryModelGroupListModel CONSTANT);
  Q_INVOKABLE void clearHistoryModelGroupList();
  Q_INVOKABLE void pushHistoryModelGroup(const QString& uid,
                                         const QString& name,
                                         const QString& image,
                                         unsigned int   price,
                                         unsigned int   creation_time,
                                         bool           collected,
                                         const QString& author_name,
                                         const QString& author_image,
                                         const QString& author_id,
                                         unsigned int   model_count);
  Q_INVOKABLE void loadHistoryModelGroupList(int page_index, int page_size);
  Q_SIGNAL void loadHistoryModelGroupListSuccessed(const QVariant& json_string,
                                                   const QVariant& page_size);

  QAbstractListModel* getRandomModelGroupListModel() const;
  Q_PROPERTY(QAbstractListModel* randomModelGroupListModel
             READ getRandomModelGroupListModel CONSTANT);
  Q_INVOKABLE void setRandomModelGroupListMaxSize(int size);
  Q_INVOKABLE void clearRandomModelGroupList();
  Q_INVOKABLE void loadRandomModelGroupList(int page_size);
  Q_SIGNAL void loadRandomModelGroupListSuccessed(const QVariant& json_string,
                                                  const QVariant& page_size);

  QAbstractListModel* getRecommendModelGroupListModel() const;
  Q_PROPERTY(QAbstractListModel* recommendModelGroupListModel
             READ getRecommendModelGroupListModel CONSTANT);
  Q_INVOKABLE void clearRecommendModelGroupList();
  Q_INVOKABLE void loadRecommendModelGroupList(int page_index, int page_size);
  Q_SIGNAL void loadRecommendModelGroupListSuccessed(const QVariant& json_string,
                                                     const QVariant& page_index);

  Q_INVOKABLE QAbstractListModel* getTypeModelGroupListModel(const QString& type_uid) const;
  Q_INVOKABLE void clearTypeModelGroupList(const QString& type_uid);
  Q_INVOKABLE void loadTypeModelGroupList(
    const QString& type_uid, int page_index, int page_size, int filter_id = 3, int pay_type = 2);
  Q_SIGNAL void loadTypeModelGroupListSuccessed(const QVariant& json_string,
                                                const QVariant& page_index);

public:  // about model group detail for single group
  /// @return ModelGroupInfoModel*
  QObject* getFocusModelGroup() const;
  Q_PROPERTY(QObject* focusModelGroup READ getFocusModelGroup CONSTANT);
  Q_INVOKABLE void setFocusModelGroupUid(const QString& group_uid);

  Q_INVOKABLE void loadModelGroupInfo(const QString& group_uid);
  Q_SIGNAL void loadModelGroupInfoSuccessed(const QVariant& json_string);

  Q_INVOKABLE void loadModelGroupFileListInfo(const QString& group_uid, int count);
  Q_SIGNAL void loadModelGroupFileListInfoSuccessed(const QVariant& json_string);

  Q_INVOKABLE void collectModelGroup(const QString& group_uid, bool collect = true);
  Q_INVOKABLE void uncollectModelGroup(const QString& group_uid);
  Q_SIGNAL void modelGroupCollected(const QVariant& group_uid);
  Q_SIGNAL void modelGroupUncollected(const QVariant& group_uid);

  Q_INVOKABLE void likeModelGroup(const QString& group_uid, bool like = true);
  Q_INVOKABLE void unlikeModelGroup(const QString& group_uid);
  Q_SIGNAL void modelGroupLiked(const QVariant& group_uid);
  Q_SIGNAL void modelGroupUnliked(const QVariant& group_uid);

  Q_INVOKABLE void loadModelGroupAuthorInfo(const QString& author_id);
  Q_SIGNAL void loadModelGroupAuthorInfoSuccessed(const QVariant& json_string);

  Q_INVOKABLE void loadModelGroupAuthorModelList(const QString& user_id, int page_index, int page_size, int filter_type, const QString& keyword);
  Q_SIGNAL void loadModelGroupAuthorModelListSuccessed(const QVariant& json_string);

  Q_INVOKABLE QString createModelGroupUrl(const QString& group_uid) const;
  Q_INVOKABLE void shareModelGroup(const QString& group_uid);
  Q_SIGNAL void shareModelGroupSuccessed();

protected:
  std::unique_ptr<ModelGroupInfoListModel>& findOrMakeTypeModel(const QString& type_uid) const;

private:
  void syncHistoryFromLocal();
  void syncHistoryToLocal() const;

private:
  QString cache_dir_path_{};
  QString history_file_path_{};

  std::function<QString(const QString&)> model_group_url_creater_{ nullptr };

  std::unique_ptr<ModelGroupCategoryListModel> category_model_{ nullptr };

  std::unique_ptr<ModelGroupInfoListModel> random_model_{ nullptr };
  std::unique_ptr<ModelGroupInfoListModel> recommend_model_{ nullptr };
  std::unique_ptr<ModelGroupInfoListModel> history_model_{ nullptr };
  std::unique_ptr<ModelGroupInfoListModel> search_model_{ nullptr };

  mutable std::map<QString, std::unique_ptr<ModelGroupInfoListModel>> type_model_map_{};
  mutable QString last_type_model_group_uid_{};

  std::unique_ptr<ModelGroupInfoModel> focus_group_{ nullptr };
};

}  // namespace cxcloud

#endif  // CXCLOUD_SERVICE_MODEL_LIBRARY_SERVICE_H_
