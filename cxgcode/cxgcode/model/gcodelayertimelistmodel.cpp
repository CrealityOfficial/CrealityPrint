#include "gcodelayertimelistmodel.h"

namespace cxgcode {

GcodeLayerTimeListModel::GcodeLayerTimeListModel(QObject* parent)
    : GcodePreviewRangeDivideModel(gcode::GCodeVisualType::gvt_layerTime, parent) {}

}  // namespace cxgcode
