#include "pickface.h"
#include "qtuser3d/trimesh2/conv.h"
#include "qtuser3d/trimesh2/renderprimitive.h"
#include "qtuser3d/trimesh2/trimeshrender.h"

PickFace::PickFace(creative_kernel::ModelN* model, const cxkernel::KernelHullFace& face, Qt3DCore::QNode* parent)
	: PurePickEntity(parent)
	, m_model(model)
	, m_face(face)
{
	m_pickable->setEnableSelect(false);

	std::vector<trimesh::vec3> tris;
	qtuser_3d::traitTriangles(m_face.mesh.get(), tris);
	setGeometry(qtuser_3d::createTriangles(tris), Qt3DRender::QGeometryRenderer::Triangles);
}

PickFace::PickFace(creative_kernel::ModelGroup* modelGroup, const cxkernel::KernelHullFace& face, Qt3DCore::QNode* parent)
	: PurePickEntity(parent)
	, m_modelGroup(modelGroup)
	, m_face(face)
{
	m_pickable->setEnableSelect(false);

	std::vector<trimesh::vec3> tris;
	qtuser_3d::traitTriangles(m_face.mesh.get(), tris);
	setGeometry(qtuser_3d::createTriangles(tris), Qt3DRender::QGeometryRenderer::Triangles);
}

PickFace::~PickFace()
{

}

trimesh::vec3 PickFace::gNormal()
{
	trimesh::fxform xf;
	if (m_model)
	{
		xf = qtuser_3d::qMatrix2Xform(m_model->globalMatrix());
	}
	else if (m_modelGroup)
	{
		xf = qtuser_3d::qMatrix2Xform(m_modelGroup->globalMatrix());
	}
	trimesh::vec n = trimesh::norm_xf(xf) * m_face.normal;
	return trimesh::normalized(n);
}

creative_kernel::ModelN* PickFace::model()
{
	return m_model;
}

creative_kernel::ModelGroup* PickFace::modelGroup()
{
	return m_modelGroup;
}