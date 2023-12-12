#ifndef CXKERNEL_SIMPLETRIANGLEENTITY_1683343013360_H
#define CXKERNEL_SIMPLETRIANGLEENTITY_1683343013360_H

#include "shader_entity_export.h"
#include "entity/pureentity.h"
#include "qtuser3d/geometry/attribute.h"

namespace qtuser_3d
{
	class SHADER_ENTITY_API SimpleTriangleEntity : public qtuser_3d::PureEntity
	{
	public:
		SimpleTriangleEntity(Qt3DCore::QNode* parent = nullptr);
		virtual ~SimpleTriangleEntity();

		void setTriangle(const trimesh::vec3& v1, const trimesh::vec3& v2, const trimesh::vec3& v3);
		void setMultiTriangles(const std::vector<trimesh::vec3>& tris);
		void setColor(const QVector4D& color);
	};
}

#endif // CXKERNEL_SIMPLETRIANGLEENTITY_1683343013360_H