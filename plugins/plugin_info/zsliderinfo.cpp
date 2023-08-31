#include "zsliderinfo.h"
#include "interface/reuseableinterface.h"

using namespace creative_kernel;
ZSliderInfo::ZSliderInfo(QObject* parent)
	: QObject(parent)
    , m_maxLayer(0.01f)
{

}

ZSliderInfo::~ZSliderInfo()
{

}

float ZSliderInfo::maxLayer()
{
    return m_maxLayer;
}

void ZSliderInfo::setTopCurrentLayer(float layer)
{
    setModelEffectClipMaxZ(layer);
}

void ZSliderInfo::setBottomCurrentLayer(float layer)
{
    setModelEffectClipBottomZ(layer);
}

void ZSliderInfo::onBoxChanged(const qtuser_3d::Box3D& box)
{

}

void ZSliderInfo::onSceneChanged(const qtuser_3d::Box3D& box)
{
    float max_z = box.max.z();
    if (qFuzzyCompare(m_maxLayer, max_z)) {
        return;
    }
    
    m_maxLayer = max_z;
    emit maxLayerChanged();
}
