#pragma once

#ifndef CXGCODE_MODEL_GCODEFANSPEEDLISTMODEL_H_
#define CXGCODE_MODEL_GCODEFANSPEEDLISTMODEL_H_

#include "cxgcode/interface.h"
#include "cxgcode/model/gcodepreviewlistmodel.h"

namespace cxgcode {


class CXGCODE_API GcodeFanSpeedListModel : public GcodePreviewRangeDivideModel {
  Q_OBJECT;

public:
  explicit GcodeFanSpeedListModel(QObject* parent = nullptr);
  virtual ~GcodeFanSpeedListModel() = default;

};

}  // namespace cxgcode

#endif  // CXGCODE_MODEL_GCODEFANSPEEDLISTMODEL_H_
