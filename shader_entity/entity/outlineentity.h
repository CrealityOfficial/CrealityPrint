#ifndef _NULLSPACE_OUTLINE_ENTITY_1591235079966_H
#define _NULLSPACE_OUTLINE_ENTITY_1591235079966_H
#include "entity/lineentity.h"
#include "shader_entity_export.h"
#include <QVector3D>
#include <QMatrix4x4>
#include "trimesh2/TriMesh.h"

namespace qtuser_3d
{
	class XEntity;
	class SHADER_ENTITY_API OutlineEntity : public XEntity
	{
		Q_OBJECT
	public:
		OutlineEntity(Qt3DCore::QNode* parent = NULL);
		virtual ~OutlineEntity();

		void setRoute(std::vector<trimesh::vec3> route);
		void setRoute(std::vector<std::vector<trimesh::vec3>> routes);

		void setOffset(float offset);

	private:
		// trimesh::vec3 triangleNormal(const trimesh::vec3& v1, const trimesh::vec3& v2, const trimesh::vec3& v3);
		// void setFloatOffset(std::vector<std::vector<trimesh::vec3>>& routes);
		void updateGeometry(int pointsNum, float* positions, bool loop);

	private:
		XEffect* m_effect;
	};
}

#endif // _NULLSPACE_OUTLINE_ENTITY_1591235079966_H