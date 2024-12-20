#pragma once

#ifndef CXGCODE_MODEL_GCODESPEEDLISTMODEL_H_
#define CXGCODE_MODEL_GCODESPEEDLISTMODEL_H_

#include <QtCore/QList>
#include <QtGui/QColor>

#include "cxgcode/interface.h"
#include "cxgcode/model/gcodepreviewlistmodel.h"

namespace cxgcode {

struct GcodeSpeedData{
  QColor color;
  double speed;
};

class CXGCODE_API GcodeSpeedListModel : public GcodePreviewListModel {
  Q_OBJECT;

public:
  explicit GcodeSpeedListModel(QObject* parent = nullptr);
  virtual ~GcodeSpeedListModel() = default;

public:
  void setDataList(const QList<GcodeSpeedData>& data_list);
  void setColors(const QList<QColor>& colors);
  void setRange(double min_speed, double max_speed);

protected:
  int rowCount(const QModelIndex& parent = QModelIndex{}) const override;
  QVariant data(const QModelIndex& index, int role = Qt::ItemDataRole::DisplayRole) const override;
  QHash<int, QByteArray> roleNames() const override;

private:
  void resetData();

private:
  enum DataRole : int {
    COLOR = Qt::ItemDataRole::UserRole + 1,
    SPEED ,
  };

  QList<GcodeSpeedData> data_list_;
  QList<QColor> colors_;
  double min_speed_;
  double max_speed_;
};

}  // namespace cxgcode

#endif  // CXGCODE_MODEL_GCODESPEEDLISTMODEL_H_
