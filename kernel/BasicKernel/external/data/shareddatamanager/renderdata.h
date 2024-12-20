#ifndef CREATIVE_KERNEL_RENDERDATA_1681019989200_H
#define CREATIVE_KERNEL_RENDERDATA_1681019989200_H
#include "data/header.h"
#include "data/kernelenum.h"
#include "trimesh2/TriMesh.h"
#include <Qt3DRender/qgeometry.h>

namespace creative_kernel
{
	class BASIC_KERNEL_API RenderData : public Qt3DCore::QNode
	{
		Q_OBJECT
	public:
		RenderData(TriMeshPtr mesh, const std::vector<std::string>& colors, int materialIndex, Qt3DCore::QNode* parent);
		virtual ~RenderData();

		Qt3DRender::QGeometry* geometry() { return m_geometry; }
		int spreadFaceCount() { return m_spreadFaceCount; }
		std::vector<int> spreadFaces() { return m_spreadFaces; }
		QSet<int> colorIndexs() { return m_colorIndexs; }

	private:
		void generate(TriMeshPtr mesh, const std::vector<std::string>& colors, int materialIndex, Qt3DCore::QNode* parent);

		Qt3DRender::QGeometry* createTrianglesWithFlags(const std::vector<trimesh::vec3>& tris, const std::vector<float>& flags, Qt3DCore::QNode* parent);

	private:
		Qt3DRender::QGeometry* m_geometry;
		QMap<int, trimesh::vec3> m_colorMap;
		QSet<int> m_colorIndexs;
		std::vector<int> m_spreadFaces;
		int m_spreadFaceCount;
	};
	typedef std::shared_ptr<RenderData> RenderDataPtr;
}

#endif // CREATIVE_KERNEL_RENDERDATA_1681019989200_H