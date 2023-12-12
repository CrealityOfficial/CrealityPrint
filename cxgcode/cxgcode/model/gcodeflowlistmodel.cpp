#include "gcodeflowlistmodel.h"

namespace cxgcode {

GcodeFlowListModel::GcodeFlowListModel(QObject* parent)
    : GcodePreviewRangeDivideModel(gcode::GCodeVisualType::gvt_flow, parent) {}

}  // namespace cxgcode
