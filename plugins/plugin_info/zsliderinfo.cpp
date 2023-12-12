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
    if (layer > (m_maxLayer - 0.1f))
        layer = m_maxLayer + 0.5f;

    setModelEffectClipMaxZ(layer);
}

void ZSliderInfo::setBottomCurrentLayer(float layer)
{
    if (layer < 0.1f)
        layer = - 0.5f;

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
	qDebug() << "m_maxLayer:" << m_maxLayer;
    emit maxLayerChanged();
}
