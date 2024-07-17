#pragma once

#ifndef CXGCODE_MODEL_GCODESTRUCTURELISTMODEL_H_
#define CXGCODE_MODEL_GCODESTRUCTURELISTMODEL_H_

#include <QtCore/QList>
#include <QtGui/QColor>

#include "crslice/gcode/define.h"
#include "cxgcode/interface.h"
#include "cxgcode/model/gcodepreviewlistmodel.h"

namespace cxgcode {

struct GcodeStructureData {
  QColor  color;
  QString name;
  QString time;
  double  percent{ 0.0 };
  int     type{ 1 };
  bool    checked{ true };
};

class CXGCODE_API GcodeStructureListModel : public GcodePreviewListModel {
  Q_OBJECT;

public:
  explicit GcodeStructureListModel(QObject* parent = nullptr);
  virtual ~GcodeStructureListModel() = default;

public:
  void setDataList(const QList<GcodeStructureData>& data_list);
  void setTimeParts(const gcode::TimeParts& time_parts);
  void setOrcaTimeParts(std::vector<std::pair<int, float>> time_parts,int total_time);
  Q_INVOKABLE void checkItem(int type, bool checked);
  Q_SIGNAL void itemCheckedChanged(int type, bool checked);

protected:
  int rowCount(const QModelIndex& parent = QModelIndex{}) const override;
  QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
  QHash<int, QByteArray> roleNames() const override;

private:
  enum DataRole : int {
    COLOR  = Qt::ItemDataRole::UserRole + 1,
    NAME   ,
    TIME   ,
    PERCENT,
    TYPE   ,
    CHECKED,
  };

  QList<GcodeStructureData> data_list_;
};

}  // namespace cxgcode

#endif  // CXGCODE_MODEL_GCODESTRUCTURELISTMODEL_H_
