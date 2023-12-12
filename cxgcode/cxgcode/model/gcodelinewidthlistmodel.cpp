#include "gcodelinewidthlistmodel.h"

namespace cxgcode {

GcodeLineWidthListModel::GcodeLineWidthListModel(QObject* parent)
    : GcodePreviewRangeDivideModel(gcode::GCodeVisualType::gvt_lineWidth, parent) {}

void GcodeLineWidthListModel::setRange(double min, double max)
{
    m_min = min;
    m_max = max;

    if (max - min <= 0.002)
    {
        setShowColorSize(1);
    }
    else {
        setShowAllColorSize();
    }
}
}  // namespace cxgcode
