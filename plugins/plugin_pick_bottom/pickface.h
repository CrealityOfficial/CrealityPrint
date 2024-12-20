#ifndef _PICKFACE_1689591426882_H
#define _PICKFACE_1689591426882_H
#include "entity/purepickentity.h"
#include "data/modeln.h"
#include "data/modelgroup.h"

class PickFace : public qtuser_3d::PurePickEntity
{
	Q_OBJECT
public:
	PickFace(creative_kernel::ModelN* model, const cxkernel::KernelHullFace& face, Qt3DCore::QNode* parent = nullptr);
	PickFace(creative_kernel::ModelGroup* modelGroup, const cxkernel::KernelHullFace& face, Qt3DCore::QNode* parent = nullptr);
	virtual ~PickFace();

	trimesh::vec3 gNormal();
	creative_kernel::ModelN* model();
	creative_kernel::ModelGroup* modelGroup();
protected:
	creative_kernel::ModelN* m_model{NULL};
	creative_kernel::ModelGroup* m_modelGroup{NULL};
	cxkernel::KernelHullFace m_face;
};
#endif // _PICKFACE_1689591426882_H