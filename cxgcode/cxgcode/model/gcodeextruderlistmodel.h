#pragma once

#ifndef CXGCODE_MODEL_GCODEEXTRUDERLISTMODEL_H_
#define CXGCODE_MODEL_GCODEEXTRUDERLISTMODEL_H_

#include <QtCore/QList>
#include <QtGui/QColor>

#include "cxgcode/interface.h"
#include "cxgcode/model/gcodepreviewlistmodel.h"

namespace cxgcode {

struct GcodeExtruderData{
  QColor color;
  int    index{ 1 };
};

class CXGCODE_API GcodeExtruderListModel : public GcodePreviewListModel {
  Q_OBJECT;

public:
  explicit GcodeExtruderListModel(QObject* parent = nullptr);
  virtual ~GcodeExtruderListModel() = default;

public:
  void setDataList(const QList<GcodeExtruderData>& data_list);

protected:
  int rowCount(const QModelIndex& parent = QModelIndex{}) const override;
  QVariant data(const QModelIndex& index, int role = Qt::ItemDataRole::DisplayRole) const override;
  QHash<int, QByteArray> roleNames() const override;

private:
  enum DataRole : int {
    COLOR = Qt::ItemDataRole::UserRole + 1,
    INDEX ,
  };

  QList<GcodeExtruderData> data_list_;
};

}  // namespace cxgcode

#endif  // CXGCODE_MODEL_GCODEEXTRUDERLISTMODEL_H_
