#pragma once

#ifndef CXGCODE_MODEL_GCODELAYERTIMELISTMODEL_H_
#define CXGCODE_MODEL_GCODELAYERTIMELISTMODEL_H_

#include "cxgcode/interface.h"
#include "cxgcode/model/gcodepreviewlistmodel.h"

namespace cxgcode {

	class CXGCODE_API GcodeLayerTimeListModel : public GcodePreviewRangeDivideModel {
		Q_OBJECT;

	public:
		explicit GcodeLayerTimeListModel(QObject* parent = nullptr);
		virtual ~GcodeLayerTimeListModel() = default;
	};

}// namespace cxgcode

#endif  // CXGCODE_MODEL_GCODELAYERTIMELISTMODEL_H_
