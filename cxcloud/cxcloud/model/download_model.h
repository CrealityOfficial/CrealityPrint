#pragma once

#ifndef CXCLOUD_MODEL_DOWNLOAD_MODEL_H_
#define CXCLOUD_MODEL_DOWNLOAD_MODEL_H_

#include <memory>

#include <QString>

#include <qtusercore/plugin/datalistmodel.h>

#include "cxcloud/interface.hpp"
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
      BROKEN      = 5,
    };

    QString  uid       { ""             };
    QString  name      { ""             };
    QString  format    { ""             }; // file suffix like ".stl" or ".3mf" (with "." char)
    QString  local_name{ ""             }; // non repeating name in local disk
    QString  image     { ""             }; // URL
    size_t   size      { 0              }; // B
    QString  date      { "0000-00-00"   }; // yyyy-MM-dd
    uint32_t speed     { 0              }; // B/s
    State    state     { State::UNREADY };
    uint32_t progress  { 0              }; // 0-100
  };

  class CXCLOUD_API DownloadItemListModel : public qtuser_qml::DataListModel<DownloadItemInfo> {
    Q_OBJECT;

   public:
    using SuperType::SuperType;

    virtual ~DownloadItemListModel() = default;

   public:
    auto getReadyCount() const -> int;
    Q_SIGNAL void readyCountChanged() const;
    Q_PROPERTY(int readyCount READ getReadyCount NOTIFY readyCountChanged);

    auto getDownloadingCount() const -> int;
    Q_SIGNAL void downloadingCountChanged() const;
    Q_PROPERTY(int downloadingCount READ getDownloadingCount NOTIFY downloadingCountChanged);

    auto getFinishedCount() const -> int;
    Q_SIGNAL void finishedCountChanged() const;
    Q_PROPERTY(int finishedCount READ getFinishedCount NOTIFY finishedCountChanged);

   public:
    auto pushFront(const Data& data) -> bool override;
    auto pushFront(Data&& data) -> bool override;

    auto pushBack(const Data& data) -> bool override;
    auto pushBack(Data&& data) -> bool override;

    auto update(const Data& data) -> bool override;
    auto update(Data&& data) -> bool override;

    auto erase(const QString& uid) -> bool override;

   protected:
    auto data(const QModelIndex& index, int role) const -> QVariant override;
    auto roleNames() const -> QHash<int, QByteArray> override;

   private:
    enum class DataRole : int {
      UID            = Qt::ItemDataRole::UserRole,
      NAME           ,
      FORMAT         ,
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

  auto operator<<(DownloadGroupInfo& info, const ModelGroupInfoModel& model) -> DownloadGroupInfo&;
  auto operator<<(DownloadGroupInfo& info, const ModelInfoListModel& model) -> DownloadGroupInfo&;

  class CXCLOUD_API DownloadGroupListModel : public qtuser_qml::DataListModel<DownloadGroupInfo> {
    Q_OBJECT;

   public:
    using SuperType::SuperType;

    virtual ~DownloadGroupListModel() = default;

   public:
    auto getReadyCount() const -> int;
    Q_SIGNAL void readyCountChanged() const;
    Q_PROPERTY(int readyCount READ getReadyCount NOTIFY readyCountChanged);

    auto getDownloadingCount() const -> int;
    Q_SIGNAL void downloadingCountChanged() const;
    Q_PROPERTY(int downloadingCount READ getDownloadingCount NOTIFY downloadingCountChanged);

    auto getFinishedCount() const -> int;
    Q_SIGNAL void finishedCountChanged() const;
    Q_PROPERTY(int finishedCount READ getFinishedCount NOTIFY finishedCountChanged);

   public:
    auto pushFront(const Data& data) -> bool override;
    auto pushFront(Data&& data) -> bool override;

    auto pushBack(const Data& data) -> bool override;
    auto pushBack(Data&& data) -> bool override;

    auto update(const Data& data) -> bool override;
    auto update(Data&& data) -> bool override;

    auto erase(const QString& uid) -> bool override;

   protected:
    auto data(const QModelIndex& index, int role) const -> QVariant override;
    auto roleNames() const -> QHash<int, QByteArray> override;

   private:
    enum class DataRole : int {
      UID   = Qt::ItemDataRole::UserRole,
      NAME  ,
      ITEMS ,
    };
  };

}  // namespace cxcloud

#endif  // !CXCLOUD_MODEL_DOWNLOAD_MODEL_H_
