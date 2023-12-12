#pragma once

#ifndef CXGCODE_MODEL_GCODETEMPERATURELISTMODEL_H_
#define CXGCODE_MODEL_GCODETEMPERATURELISTMODEL_H_

#include <QtCore/QList>
#include <QtGui/QColor>

#include "cxgcode/interface.h"
#include "cxgcode/model/gcodepreviewlistmodel.h"

namespace cxgcode {

class CXGCODE_API GcodeTemperatureListModel : public GcodePreviewRangeDivideModel {
  Q_OBJECT;

public:
  explicit GcodeTemperatureListModel(QObject* parent = nullptr);
  virtual ~GcodeTemperatureListModel() = default;

  void setRange(double min, double max) override;

};

}  // namespace cxgcode

#endif  // CXGCODE_MODEL_GCODETEMPERATURELISTMODEL_H_
