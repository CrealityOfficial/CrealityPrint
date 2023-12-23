#pragma once

#ifndef CXCLOUD_MODEL_MODEL_INFO_MODEL_H_
#define CXCLOUD_MODEL_MODEL_INFO_MODEL_H_

#include <memory>
#include <vector>

#include <QtCore/QAbstractListModel>
#include <QtCore/QPointer>
#include <QtCore/QString>

#include "cxcloud/interface.hpp"
#include "cxcloud/model/base_model.h"

namespace cxcloud {

struct ModelGroupCategoryInfo {
  QString uid{};
  QString name{};
};

class CXCLOUD_API ModelGroupCategoryListModel : public DataListModel<ModelGroupCategoryInfo> {
  Q_OBJECT;

public:
  using SuperType::SuperType;

  virtual ~ModelGroupCategoryListModel() = default;

public:
  Q_INVOKABLE QVariantMap get(int index) const override;

protected:
  QVariant data(const QModelIndex& index, int role = Qt::ItemDataRole::DisplayRole) const override;
  QHash<int, QByteArray> roleNames() const override;

private:
  enum class DataRole : int {
    UID = Qt::ItemDataRole::UserRole + 1,
    NAME,
  };
};

struct ModelInfo {
  QString uid{};
  QString name{};
  QString image{};
  size_t size{};
};

class CXCLOUD_API ModelInfoListModel : public DataListModel<ModelInfo> {
  Q_OBJECT;

public:
  using Info = Data;

public:
  explicit ModelInfoListModel(QObject* parent = nullptr);
  virtual ~ModelInfoListModel() = default;

protected:
  QVariant data(const QModelIndex& index, int role = Qt::ItemDataRole::DisplayRole) const override;
  QHash<int, QByteArray> roleNames() const override;

private:
  enum class DataRole : int {
    UID           = Qt::ItemDataRole::UserRole + 1,
    NAME          ,
    IMAGE         ,
    SIZE          ,
    SIZE_WITH_UNIT,
  };
};

struct ModelGroupInfo {
  QString uid{};
  QString name{};
  QString image{};
  QString license{};
  size_t price{ 0ull };
  size_t creation_time{ 0ull };  // seconds timestamp
  bool collected{ false };
  bool liked{ false };
  QString author_name{};
  QString author_image{};
  QString author_id{};
  size_t model_count{ 0ull };
  std::shared_ptr<ModelInfoListModel> models{ nullptr };
};

class CXCLOUD_API ModelGroupInfoListModel : public DataListModel<ModelGroupInfo> {
  Q_OBJECT;

public:
  using SuperType::SuperType;

  virtual ~ModelGroupInfoListModel() = default;

protected:
  QVariant data(const QModelIndex& index, int role = Qt::ItemDataRole::DisplayRole) const override;
  QHash<int, QByteArray> roleNames() const override;

private:
  enum class DataRole : int {
    UID          = Qt::ItemDataRole::UserRole + 1,
    NAME         ,
    IMAGE        ,
    LICENSE      ,
    PRICE        ,
    CREATION_TIME,
    COLLECTED    ,
    LIKED        ,
    AUTHOR_NAME  ,
    AUTHOR_IMAGE ,
    MODEL_COUNT  ,
    MODELS       ,
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
  QString getUid() const;
  void setUid(const QString& uid);
  Q_SIGNAL void uidChanged();
  Q_PROPERTY(QString uid READ getUid NOTIFY uidChanged);

public:
  QString getName() const;
  void setName(const QString& name);
  Q_SIGNAL void nameChanged();
  Q_PROPERTY(QString name READ getName NOTIFY nameChanged);

public:
  QString getImage() const;
  void setImage(const QString& image);
  Q_SIGNAL void imageChanged();
  Q_PROPERTY(QString image READ getImage NOTIFY imageChanged);

public:
  QString getLicense() const;
  void setLicense(const QString& license);
  Q_SIGNAL void licenseChanged();
  Q_PROPERTY(QString license READ getLicense NOTIFY licenseChanged);

public:
  int getPrice() const;
  void setPrice(int price);
  Q_SIGNAL void priceChanged();
  Q_PROPERTY(int price READ getPrice NOTIFY priceChanged);

public:
  /// @return seconds timestamp
  int getCreationTime() const;
  void setCreationTime(int timestamp);
  Q_SIGNAL void creationTimeChanged();
  Q_PROPERTY(int creationTime READ getCreationTime NOTIFY creationTimeChanged);

public:
  bool isCollected() const;
  void setCollected(bool collected);
  Q_SIGNAL void collectedChanged();
  Q_PROPERTY(bool collected READ isCollected NOTIFY collectedChanged);

public:
  bool isLiked() const;
  void setLiked(bool liked);
  Q_SIGNAL void likedChanged();
  Q_PROPERTY(bool liked READ isLiked NOTIFY likedChanged);

public:
  QString getAuthorName() const;
  void setAuthorName(const QString& name);
  Q_SIGNAL void authorNameChanged();
  Q_PROPERTY(QString authorName READ getAuthorName NOTIFY authorNameChanged);

public:
  QString getAuthorImage() const;
  void setAuthorImage(const QString& image);
  Q_SIGNAL void authorImageChanged();
  Q_PROPERTY(QString authorImage READ getAuthorImage NOTIFY authorImageChanged);

public:
  int getModelCount() const;
  void setModelCount(int count);
  Q_SIGNAL void modelCountChanged();
  Q_PROPERTY(int modelCount READ getModelCount NOTIFY modelCountChanged);

public:
  std::weak_ptr<ModelInfoListModel> models() const;
  QAbstractListModel* getModels() const;
  Q_PROPERTY(QAbstractListModel* models READ getModels CONSTANT);

protected:
  std::shared_ptr<ModelInfoListModel> loadOrMakeModels() const;

private:
  mutable ModelGroupInfo info_;
};

}  // namespace cxcloud

#endif  // CXCLOUD_MODEL_MODEL_INFO_MODEL_H_
