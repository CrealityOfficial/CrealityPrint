#ifndef _PICKFACE_1689591426882_H
#define _PICKFACE_1689591426882_H
#include "entity/purepickentity.h"
#include "data/modeln.h"

class PickFace : public qtuser_3d::PurePickEntity
{
	Q_OBJECT
public:
	PickFace(creative_kernel::ModelN* model, const cxkernel::KernelHullFace& face, Qt3DCore::QNode* parent = nullptr);
	virtual ~PickFace();

	trimesh::vec3 gNormal();
	creative_kernel::ModelN* model();
protected:
	creative_kernel::ModelN* m_model;
	cxkernel::KernelHullFace m_face;
};
#endif // _PICKFACE_1689591426882_H