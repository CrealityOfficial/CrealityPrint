#pragma once

#ifndef CXCLOUD_MODEL_SERVER_LIST_MODEL_H_
#define CXCLOUD_MODEL_SERVER_LIST_MODEL_H_

#include <qtusercore/plugin/datalistmodel.h>

#include "cxcloud/interface.hpp"

namespace cxcloud {

  struct ServerInfo {
    QString uid{};  // string of ServerType
    QString name{};
  };

  class CXCLOUD_API ServerListModel : public qtuser_qml::DataListModel<ServerInfo> {
    Q_OBJECT;

   public:
    using SuperType::SuperType;

    virtual ~ServerListModel() = default;

   public:
    Q_INVOKABLE QVariantMap get(int index) const override;

   protected:
    auto data(const QModelIndex& index, int role) const -> QVariant override;
    auto roleNames() const -> QHash<int, QByteArray> override;

   private:
    enum class DataRole : int {
      UID = Qt::ItemDataRole::UserRole,
      INDEX,
      NAME,
    };
  };

}  // namespace cxcloud

#endif  // CXCLOUD_MODEL_SERVER_LIST_MODEL_H_
