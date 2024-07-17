#pragma once

#ifndef CXCLOUD_MODEL_SLICE_MODEL_H_
#define CXCLOUD_MODEL_SLICE_MODEL_H_

#include <QtCore/QStringList>

#include <qtusercore/plugin/datalistmodel.h>

#include "cxcloud/interface.hpp"

namespace cxcloud {

  struct SliceInfo {
    QString uid;
    QString name;
    QString image;
    QString suffix;
    QString url;
  };

  class CXCLOUD_API SliceListModel : public qtuser_qml::DataListModel<SliceInfo> {
    Q_OBJECT;

   public:
    using SuperType::SuperType;
    explicit SliceListModel(const QStringList& vaild_suffix_list, QObject* parent = nullptr);
    explicit SliceListModel(QStringList&& vaild_suffix_list, QObject* parent = nullptr);

    virtual ~SliceListModel() = default;

   public:
    /// @brief Only vaild suffix will pushBack into list.
    ///        If the list is empty, it means all suffix is vaild.
    auto getVaildSuffixList() const -> QStringList;
    auto setVaildSuffixList(const QStringList& vaild_suffix_list) -> void;

   public:
    Q_INVOKABLE QVariantMap get(int index) const override;

   protected:
    auto pushBack(const Data& data) -> bool override;
    auto pushBack(Data&& data) -> bool override;

   protected:
    auto data(const QModelIndex& index, int role) const -> QVariant override;
    auto roleNames() const -> QHash<int, QByteArray> override;

   private:
    enum class DataRole : int {
      UID = Qt::ItemDataRole::UserRole,
      NAME,
      IMAGE,
      SUFFIX,
      URL,
    };

    QStringList vaild_suffix_list_;
  };

}  // namespace cxcloud

#endif
