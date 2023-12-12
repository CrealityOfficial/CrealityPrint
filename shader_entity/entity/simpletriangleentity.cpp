#include "simpletriangleentity.h"
#include "qtuser3d/trimesh2/renderprimitive.h"

namespace qtuser_3d
{
	SimpleTriangleEntity::SimpleTriangleEntity(Qt3DCore::QNode* parent)
		: PureEntity(parent)
	{
		setParameter("color", QVector4D(0.6f, 0.6f, 0.0f, 1.0f));
	}

	SimpleTriangleEntity::~SimpleTriangleEntity()
	{

	}

	void SimpleTriangleEntity::setTriangle(const trimesh::vec3& v1, const trimesh::vec3& v2, const trimesh::vec3& v3)
	{
		setGeometry(createSimpleTriangle(v1, v2, v3));
	}

	void SimpleTriangleEntity::setMultiTriangles(const std::vector<trimesh::vec3>& tris)
	{
		setGeometry(createTriangles(tris));
	}

	void SimpleTriangleEntity::setColor(const QVector4D& color)
	{
		setParameter("color", color);
	}
}