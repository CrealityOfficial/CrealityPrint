#pragma once

#ifndef CXGCODE_MODEL_GCODELINEWIDTHLISTMODEL_H_
#define CXGCODE_MODEL_GCODELINEWIDTHLISTMODEL_H_

#include "cxgcode/interface.h"
#include "cxgcode/model/gcodepreviewlistmodel.h"

namespace cxgcode {

class CXGCODE_API GcodeLineWidthListModel : public GcodePreviewRangeDivideModel {
  Q_OBJECT;

public:
  explicit GcodeLineWidthListModel(QObject* parent = nullptr);
  virtual ~GcodeLineWidthListModel() = default;

  void setRange(double min, double max) override;
};

}  // namespace cxgcode

#endif  // CXGCODE_MODEL_GCODELINEWIDTHLISTMODEL_H_
