#pragma once

#ifndef CXCLOUD_MODEL_MODEL_INFO_MODEL_H_
#define CXCLOUD_MODEL_MODEL_INFO_MODEL_H_

#include <memory>

#include <QtCore/QAbstractListModel>
#include <QtCore/QString>

#include <qtusercore/plugin/datalistmodel.h>

#include "cxcloud/interface.hpp"
#include "cxcloud/model/project_model.h"

namespace cxcloud {

  struct ModelGroupCategoryInfo {
    QString uid{};
    QString name{};
  };

  class CXCLOUD_API ModelGroupCategoryListModel
      : public qtuser_qml::DataListModel<ModelGroupCategoryInfo> {
    Q_OBJECT;

   public:
    using SuperType::SuperType;

    virtual ~ModelGroupCategoryListModel() = default;

   public:
    Q_INVOKABLE QVariantMap get(int index) const override;

   protected:
    auto data(const QModelIndex& index, int role) const -> QVariant override;
    auto roleNames() const -> QHash<int, QByteArray> override;

   private:
    enum class DataRole : int {
      UID = Qt::ItemDataRole::UserRole,
      NAME,
    };
  };





  struct ModelInfo {
    QString uid{};
    QString name{};
    QString format{};
    QString image{};
    size_t  size{ 0ull };
    bool    broken{ false };
  };

  class CXCLOUD_API ModelInfoListModel : public qtuser_qml::DataListModel<ModelInfo> {
    Q_OBJECT;

   public:
    using Info = Data;

   public:
    using SuperType::SuperType;

    virtual ~ModelInfoListModel() = default;

   protected:
    auto data(const QModelIndex& index, int role) const -> QVariant override;
    auto roleNames() const -> QHash<int, QByteArray> override;

   private:
    enum class DataRole : int {
      UID           = Qt::ItemDataRole::UserRole,
      NAME          ,
      FORMAT        ,
      IMAGE         ,
      SIZE          ,
      SIZE_WITH_UNIT,
      BROKEN        ,
    };
  };





  struct ModelGroupInfo {
    enum class Status : int {
      REVIEW_UNFINISHED = 1,
      REVIEW_PASSED = 2,
      REVIEW_FAILED = 3,
      REMOVED = 4,
      DELETED = 5,
    };

    static const std::map<Status, QString> STATUS_STRING_MAP;
    static const std::map<QString, Status> STRING_STATUS_MAP;

    QString uid{};
    QString name{};
    QString image{};
    QString license{};
    size_t price{ 0ull };
    size_t creation_time{ 0ull };  // seconds timestamp
    Status status{ Status::REVIEW_PASSED };
    bool collected{ false };
    bool liked{ false };
    bool purchased{ false };
    bool restricted{ false };
    QString author_name{};
    QString author_image{};
    QString author_id{};
    size_t model_count{ 0ull };
    size_t model_broken_count{ 0ull };
    std::shared_ptr<ModelInfoListModel> models{ nullptr };
    size_t project_count{ 0ull };
    std::shared_ptr<ProjectListModel> projects{ nullptr };
  };

  class CXCLOUD_API ModelGroupInfoListModel : public qtuser_qml::DataListModel<ModelGroupInfo> {
    Q_OBJECT;

   public:
    using SuperType::SuperType;

    virtual ~ModelGroupInfoListModel() = default;

   protected:
    auto data(const QModelIndex& index, int role) const -> QVariant override;
    auto roleNames() const -> QHash<int, QByteArray> override;

   private:
    enum class DataRole : int {
      UID               = Qt::ItemDataRole::UserRole,
      NAME              ,
      IMAGE             ,
      LICENSE           ,
      PRICE             ,
      CREATION_TIME     ,
      STATUS            ,
      COLLECTED         ,
      LIKED             ,
      PURCHASED         ,
      RESTRICTED        ,
      AUTHOR_NAME       ,
      AUTHOR_IMAGE      ,
      MODEL_COUNT       ,
      MODEL_BROKEN_COUNT,
      MODELS            ,
      PROJECT_COUNT     ,
      PROJECTS          ,
    };
  };





  class CXCLOUD_API ModelGroupInfoModel : public QObject {
    Q_OBJECT;

   public:
    explicit ModelGroupInfoModel(const ModelGroupInfo& info, QObject* parent = nullptr);
    explicit ModelGroupInfoModel(ModelGroupInfo&& info, QObject* parent = nullptr);
    explicit ModelGroupInfoModel(QObject* parent = nullptr);
    virtual ~ModelGroupInfoModel() = default;

   public:
    auto getUid() const -> QString;
    auto setUid(const QString& uid) -> void;
    Q_SIGNAL void uidChanged();
    Q_PROPERTY(QString uid READ getUid NOTIFY uidChanged);

   public:
    auto getName() const -> QString;
    auto setName(const QString& name) -> void;
    Q_SIGNAL void nameChanged();
    Q_PROPERTY(QString name READ getName NOTIFY nameChanged);

   public:
    auto getImage() const -> QString;
    auto setImage(const QString& image) -> void;
    Q_SIGNAL void imageChanged();
    Q_PROPERTY(QString image READ getImage NOTIFY imageChanged);

   public:
    auto getLicense() const -> QString;
    auto setLicense(const QString& license) -> void;
    Q_SIGNAL void licenseChanged();
    Q_PROPERTY(QString license READ getLicense NOTIFY licenseChanged);

   public:
    auto getPrice() const -> int;
    auto setPrice(int price) -> void;
    Q_SIGNAL void priceChanged();
    Q_PROPERTY(int price READ getPrice NOTIFY priceChanged);

   public:
    /// @return seconds timestamp
    auto getCreationTime() const -> int;
    auto setCreationTime(int timestamp) -> void;
    Q_SIGNAL void creationTimeChanged();
    Q_PROPERTY(int creationTime READ getCreationTime NOTIFY creationTimeChanged);

   public:
    auto getStatus() const -> ModelGroupInfo::Status;
    auto setStatus(ModelGroupInfo::Status status) -> void;
    auto getStatusString() const -> QString;
    Q_SIGNAL void statusChanged();
    Q_PROPERTY(QString status READ getStatusString NOTIFY statusChanged);

   public:
    auto isCollected() const -> bool;
    auto setCollected(bool collected) -> void;
    Q_SIGNAL void collectedChanged();
    Q_PROPERTY(bool collected READ isCollected NOTIFY collectedChanged);

   public:
    auto isLiked() const -> bool;
    auto setLiked(bool liked) -> void;
    Q_SIGNAL void likedChanged();
    Q_PROPERTY(bool liked READ isLiked NOTIFY likedChanged);

   public:
    auto isPurchased() const -> bool;
    auto setPurchased(bool purchased) -> void;
    Q_SIGNAL void purchasedChanged();
    Q_PROPERTY(bool purchased READ isPurchased NOTIFY purchasedChanged);

   public:
    auto isRestricted() const -> bool;
    auto setRestricted(bool restricted) -> void;
    Q_SIGNAL void restrictedChanged();
    Q_PROPERTY(bool restricted READ isRestricted NOTIFY restrictedChanged);

   public:
    auto getAuthorName() const -> QString;
    auto setAuthorName(const QString& name) -> void;
    Q_SIGNAL void authorNameChanged();
    Q_PROPERTY(QString authorName READ getAuthorName NOTIFY authorNameChanged);

   public:
    auto getAuthorImage() const -> QString;
    auto setAuthorImage(const QString& image) -> void;
    Q_SIGNAL void authorImageChanged();
    Q_PROPERTY(QString authorImage READ getAuthorImage NOTIFY authorImageChanged);

   public:
    auto getModelCount() const -> int;
    auto setModelCount(int count) -> void;
    Q_SIGNAL void modelCountChanged();
    Q_PROPERTY(int modelCount READ getModelCount NOTIFY modelCountChanged);

   public:
    auto getModelBrokenCount() const -> int;
    auto setModelBrokenCount(int count) -> void;
    Q_SIGNAL void modelBrokenCountChanged();
    Q_PROPERTY(int modelBrokenCount READ getModelBrokenCount NOTIFY modelBrokenCountChanged);

   public:
    auto models() const -> std::weak_ptr<ModelInfoListModel>;
    auto getModels() const -> QAbstractListModel*;
    Q_PROPERTY(QAbstractListModel* models READ getModels CONSTANT);

   public:
    auto getProjectCount() const -> int;
    auto setProjectCount(int count) -> void;
    Q_SIGNAL void projectCountChanged();
    Q_PROPERTY(int projectCount READ getProjectCount NOTIFY projectCountChanged);

   public:
    auto projects() const -> std::weak_ptr<ProjectListModel>;
    auto getProjects() const -> QAbstractListModel*;
    Q_PROPERTY(QAbstractListModel* projects READ getProjects CONSTANT);

   protected:
    auto loadOrMakeModels() const -> std::shared_ptr<ModelInfoListModel>;
    auto loadOrMakeProjects() const -> std::shared_ptr<ProjectListModel>;

   private:
    mutable ModelGroupInfo info_{};
  };

}  // namespace cxcloud

#endif  // CXCLOUD_MODEL_MODEL_INFO_MODEL_H_
