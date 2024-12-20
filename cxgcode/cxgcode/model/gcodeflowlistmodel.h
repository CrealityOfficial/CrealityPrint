#pragma once

#ifndef CXGCODE_MODEL_GCODEFLOWLISTMODEL_H_
#define CXGCODE_MODEL_GCODEFLOWLISTMODEL_H_

#include "cxgcode/interface.h"
#include "cxgcode/model/gcodepreviewlistmodel.h"

namespace cxgcode {


class CXGCODE_API GcodeFlowListModel : public GcodePreviewRangeDivideModel {
  Q_OBJECT;

public:
  explicit GcodeFlowListModel(QObject* parent = nullptr);
  virtual ~GcodeFlowListModel() = default;

};

}  // namespace cxgcode

#endif  // CXGCODE_MODEL_GCODEFLOWLISTMODEL_H_
