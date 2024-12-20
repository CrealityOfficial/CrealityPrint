#include "finephongentity.h"
#include "qtuser3d/trimesh2/renderprimitive.h"
#include "effect/finephongeffect.h"

namespace qtuser_3d
{
	FinePhongEntity::FinePhongEntity(Qt3DCore::QNode* parent)
		: XEntity(parent)
	{
		setEffect(new FinePhongEffect());
		setParameter("ambient", QVector4D(0.4f, 0.4f, 0.4f, 1.0f));
	}

	FinePhongEntity::~FinePhongEntity()
	{

	}


	void FinePhongEntity::setMultiTriangles(const std::vector<trimesh::vec3>& tris, const std::vector<trimesh::vec3> normals)
	{
		setGeometry(createTrianglesWithNormals(tris, normals));
	}
	
	void FinePhongEntity::setColor(const QVector4D& color)
	{
		setParameter("color", color);
	}

	void FinePhongEntity::setLightDirection(const QVector3D& lightDirection)
	{
		setParameter("u_lightDir", lightDirection);
	}
}