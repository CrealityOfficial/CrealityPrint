#ifndef _ZSLIDERINFO_H
#define _ZSLIDERINFO_H
#include "data/interface.h"

class ZSliderInfo: public QObject, public creative_kernel::SpaceTracer
{
	Q_OBJECT
    Q_PROPERTY(float maxLayer READ maxLayer NOTIFY maxLayerChanged)

public:
    ZSliderInfo(QObject* parent = nullptr);
    virtual ~ZSliderInfo();

    float maxLayer();

    Q_INVOKABLE void setTopCurrentLayer(float layer);
    Q_INVOKABLE void setBottomCurrentLayer(float layer);

protected:
    void onSceneChanged(const trimesh::dbox3& box) override;

signals:
    void maxLayerChanged();

protected:
    float m_maxLayer;
};
#endif // _ZSLIDERINFO_H
