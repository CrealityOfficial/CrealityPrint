#pragma once

#ifndef CXCLOUD_MODEL_SLICE_MODEL_H_
#define CXCLOUD_MODEL_SLICE_MODEL_H_

#include <QtCore/QStringList>

#include "cxcloud/interface.hpp"
#include "cxcloud/model/base_model.h"

namespace cxcloud {

struct SliceInfo {
  QString uid;
  QString name;
  QString image;
  QString suffix;
  QString url;
};

class CXCLOUD_API SliceListModel : public DataListModel<SliceInfo> {
  Q_OBJECT;

public:
  using SuperType::SuperType;
  explicit SliceListModel(const QStringList& vaild_suffix_list, QObject* parent = nullptr);
  explicit SliceListModel(QStringList&& vaild_suffix_list, QObject* parent = nullptr);

  virtual ~SliceListModel() = default;

public:
  /// @brief Only vaild suffix will pushBack into list.
  ///        If the list is empty, it means all suffix is vaild.
  QStringList getVaildSuffixList() const;
  void setVaildSuffixList(const QStringList& vaild_suffix_list);

public:
  Q_INVOKABLE QVariantMap get(int index) const override;

protected:
  auto pushBack(const Data& data) -> bool override;
  auto pushBack(Data&& data) -> bool override;

protected:
  QVariant data(const QModelIndex& index, int role = Qt::ItemDataRole::DisplayRole) const override;
  QHash<int, QByteArray> roleNames() const override;

private:
  enum class DataRole : int {
    UID = Qt::ItemDataRole::UserRole + 1,
    NAME,
    IMAGE,
    SUFFIX,
    URL,
  };

  QStringList vaild_suffix_list_;
};

}  // namespace cxcloud

#endif
