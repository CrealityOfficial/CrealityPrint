#ifndef CXKERNEL_FINEPHONGENTITY_1683343013360_H
#define CXKERNEL_FINEPHONGENTITY_1683343013360_H

#include "shader_entity_export.h"
#include "qtuser3d/refactor/xentity.h"
#include "qtuser3d/geometry/attribute.h"

namespace qtuser_3d
{
	class SHADER_ENTITY_API FinePhongEntity : public qtuser_3d::XEntity
	{
	public:
		FinePhongEntity(Qt3DCore::QNode* parent = nullptr);
		virtual ~FinePhongEntity();

		void setMultiTriangles(const std::vector<trimesh::vec3>& tris, const std::vector<trimesh::vec3> normals);
		void setColor(const QVector4D& color);
		void setLightDirection(const QVector3D& lightDirection);
	};
}

#endif // CXKERNEL_FINEPHONGENTITY_1683343013360_H