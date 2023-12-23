#pragma once

#ifndef CXCLOUD_MODEL_DOWNLOAD_MODEL_H_
#define CXCLOUD_MODEL_DOWNLOAD_MODEL_H_

#include <vector>
#include <memory>

#include <QAbstractListModel>
#include <QString>

#include "cxcloud/interface.hpp"
#include "cxcloud/model/base_model.h"
#include "cxcloud/model/model_info_model.h"

namespace cxcloud {

struct DownloadItemInfo {
  enum class State : int {
    UNREADY     = -1,
    READY       = 0,
    DOWNLOADING = 1,
    PAUSED      = 2,
    FAILED      = 3,
    FINISHED    = 4,
  };

  QString  uid       { ""             };
  QString  name      { ""             };
  QString  local_name{ ""             }; // non repeating name in local disk
  QString  image     { ""             }; // URL
  size_t   size      { 0              }; // B
  QString  date      { "0000-00-00"   }; // yyyy-MM-dd
  uint32_t speed     { 0              }; // B/s
  State    state     { State::UNREADY };
  uint32_t progress  { 0              }; // 0-100
};

class CXCLOUD_API DownloadItemListModel : public DataListModel<DownloadItemInfo> {
  Q_OBJECT;

public:
  using SuperType::SuperType;

  virtual ~DownloadItemListModel() = default;

public:
  Q_PROPERTY(int readyCount READ getReadyCount NOTIFY readyCountChanged);
  Q_SIGNAL void readyCountChanged() const;
  Q_INVOKABLE int getReadyCount() const;

  Q_PROPERTY(int downloadingCount READ getDownloadingCount NOTIFY downloadingCountChanged);
  Q_SIGNAL void downloadingCountChanged() const;
  Q_INVOKABLE int getDownloadingCount() const;

  Q_PROPERTY(int finishedCount READ getFinishedCount NOTIFY finishedCountChanged);
  Q_SIGNAL void finishedCountChanged() const;
  Q_INVOKABLE int getFinishedCount() const;

public:
  auto pushFront(const Data& data) -> bool override;
  auto pushFront(Data&& data) -> bool override;

  auto pushBack(const Data& data) -> bool override;
  auto pushBack(Data&& data) -> bool override;

  auto update(const Data& data) -> bool override;
  auto update(Data&& data) -> bool override;

  auto erase(const QString& uid) -> bool override;

protected:
  QVariant data(const QModelIndex& index, int role = Qt::ItemDataRole::DisplayRole) const override;
  QHash<int, QByteArray> roleNames() const override;

private:
  enum class DataRole : int {
    UID            = Qt::ItemDataRole::UserRole + 1,
    NAME           ,
    IMAGE          ,
    SIZE           ,
    SIZE_WITH_UNIT ,
    DATE           ,
    SPEED          ,
    SPEED_WITH_UNIT,
    STATE          ,
    PROGRESS       ,
  };
};





struct DownloadGroupInfo {
  QString uid                                 { ""      };
  QString name                                { ""      };
  QString local_name                          { ""      }; // non repeating name in local disk
  std::shared_ptr<DownloadItemListModel> items{ nullptr };
};

DownloadGroupInfo& operator<<(DownloadGroupInfo& info, const ModelGroupInfoModel& model);
DownloadGroupInfo& operator<<(DownloadGroupInfo& info, const ModelInfoListModel& model);

class CXCLOUD_API DownloadGroupListModel : public DataListModel<DownloadGroupInfo> {
  Q_OBJECT;

public:
  using SuperType::SuperType;

  virtual ~DownloadGroupListModel() = default;

public:
  Q_PROPERTY(int readyCount READ getReadyCount NOTIFY readyCountChanged);
  Q_SIGNAL void readyCountChanged() const;
  Q_INVOKABLE int getReadyCount() const;

  Q_PROPERTY(int downloadingCount READ getDownloadingCount NOTIFY downloadingCountChanged);
  Q_SIGNAL void downloadingCountChanged() const;
  Q_INVOKABLE int getDownloadingCount() const;

  Q_PROPERTY(int finishedCount READ getFinishedCount NOTIFY finishedCountChanged);
  Q_SIGNAL void finishedCountChanged() const;
  Q_INVOKABLE int getFinishedCount() const;

public:
  auto pushFront(const Data& data) -> bool override;
  auto pushFront(Data&& data) -> bool override;

  auto pushBack(const Data& info) -> bool override;
  auto pushBack(Data&& info) -> bool override;

  auto update(const Data& info) -> bool override;
  auto update(Data&& info) -> bool override;

  auto erase(const QString& uid) -> bool override;

protected:
  QVariant data(const QModelIndex& index, int role = Qt::ItemDataRole::DisplayRole) const override;
  QHash<int, QByteArray> roleNames() const override;

private:
  enum class DataRole : int {
    UID  = Qt::ItemDataRole::UserRole + 1,
    NAME ,
    ITEMS,
  };
};

} // namespace cxcloud

#endif // !CXCLOUD_MODEL_DOWNLOAD_MODEL_H_
