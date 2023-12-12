#pragma once

#ifndef CXGCODE_MODEL_GCODELAYERHIGHTLISTMODEL_H_
#define CXGCODE_MODEL_GCODELAYERHIGHTLISTMODEL_H_

#include "cxgcode/interface.h"
#include "cxgcode/model/gcodepreviewlistmodel.h"

namespace cxgcode {

class CXGCODE_API GcodeLayerHightListModel : public GcodePreviewRangeDivideModel {
  Q_OBJECT;

public:
  explicit GcodeLayerHightListModel(QObject* parent = nullptr);
  virtual ~GcodeLayerHightListModel() = default;

  void setRange(double min, double max) override;

protected:
	
};
}  // namespace cxgcode

#endif  // CXGCODE_MODEL_GCODELAYERHIGHTLISTMODEL_H_
