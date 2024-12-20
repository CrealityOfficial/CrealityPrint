#ifndef _NULLSPACE_SPLITPLANE_1592443125497_H
#define _NULLSPACE_SPLITPLANE_1592443125497_H
#include "qtuser3d/refactor/xentity.h"
#include "qtuser3d/math/box3d.h"

namespace creative_kernel
{
	class ModelN;
}

class SplitPlane: public qtuser_3d::XEntity
{
public:
	SplitPlane(Qt3DCore::QNode* parent = nullptr);
	virtual ~SplitPlane();

	void updateBox(qtuser_3d::Box3D box);

	void setGlobalNormal(const QVector3D& normal);

	void setTargetModel(creative_kernel::ModelN* model);

protected:
	Qt3DRender::QParameter* m_color;
};
#endif // _NULLSPACE_SPLITPLANE_1592443125497_H
