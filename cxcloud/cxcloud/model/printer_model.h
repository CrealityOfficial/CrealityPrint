#pragma once

#ifndef CXCLOUD_MODEL_PRINTER_MODEL_H_
#define CXCLOUD_MODEL_PRINTER_MODEL_H_

#include <QtCore/QStringList>

#include <qtusercore/plugin/datalistmodel.h>

#include "cxcloud/interface.hpp"

namespace cxcloud {

  struct PrinterInfo {
    QString uid{};
    QString ip{};
    QString image{};
    QString nickname{};
    QString device_id{};
    QString device_name{};
    bool online{ false };
    bool connected{ false };
    bool tf_card_exisit{ false };
  };

  class CXCLOUD_API PrinterListModel : public qtuser_qml::DataListModel<PrinterInfo> {
    Q_OBJECT;

   public:
    using SuperType::SuperType;

    virtual ~PrinterListModel() = default;

   public:
    Q_INVOKABLE QVariantMap get(int index) const override;

   protected:
    auto data(const QModelIndex& index, int role) const -> QVariant override;
    auto roleNames() const -> QHash<int, QByteArray> override;

   private:
    enum class DataRole : int {
      UID = Qt::ItemDataRole::UserRole,
      IP,
      IMAGE,
      NICKNAME,
      DEVICE_ID,
      DEVICE_NAME,
      ONLINE,
      CONNECTED,
      TF_CARD_EXISIT,
    };
  };

}  // namespace cxcloud

#endif  // CXCLOUD_MODEL_PRINTER_MODEL_H_
