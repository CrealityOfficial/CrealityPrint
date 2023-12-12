#include "gcodefanspeedlistmodel.h"

namespace cxgcode {

GcodeFanSpeedListModel::GcodeFanSpeedListModel(QObject* parent)
    : GcodePreviewRangeDivideModel(gcode::GCodeVisualType::gvt_fanSpeed, parent) {}

}  // namespace cxgcode
