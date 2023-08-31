#include "pickface.h"
#include "qcxutil/trimesh2/conv.h"
#include "mmesh/trimesh/trimeshrender.h"
#include "qcxutil/trimesh2/renderprimitive.h"

PickFace::PickFace(creative_kernel::ModelN* model, const qhullWrapper::HullFace& face, Qt3DCore::QNode* parent)
	: PurePickEntity(parent)
	, m_model(model)
	, m_face(face)
{
	m_pickable->setEnableSelect(false);

	std::vector<trimesh::vec3> tris;
	mmesh::traitTriangles(m_face.mesh.get(), tris);
	setGeometry(qcxutil::createTriangles(tris), Qt3DRender::QGeometryRenderer::Triangles);
}

PickFace::~PickFace()
{

}

trimesh::vec3 PickFace::gNormal()
{
	trimesh::fxform xf = qcxutil::qMatrix2Xform(m_model->globalMatrix());
	trimesh::vec n = trimesh::norm_xf(xf) * m_face.normal;
	return trimesh::normalized(n);
}

creative_kernel::ModelN* PickFace::model()
{
	return m_model;
}