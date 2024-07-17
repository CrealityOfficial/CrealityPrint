#include "outlineentity.h"
#include "qtuser3d/refactor/xrenderpass.h"
#include "qtuser3d/refactor/xentity.h"
#include "qtuser3d/geometry/linecreatehelper.h"
#include <Qt3DRender/QLineWidth>
#include <Qt3DRender/QPolygonOffset>
#include "qtuser3d/refactor/xeffect.h"

namespace qtuser_3d {
	OutlineEntity::OutlineEntity(Qt3DCore::QNode* parent) : XEntity(parent)
	{
		m_effect = new XEffect(this);

		qtuser_3d::XRenderPass* pass = new qtuser_3d::XRenderPass("outline");
		pass->setParameter("color", QVector4D(1.0f, 0.7529f, 0.0f, 1.0));
		pass->addFilterKeyMask("alpha", 0);
		pass->setPassDepthTest();
		pass->setPassCullFace();

		Qt3DRender::QLineWidth* lineWidth = new Qt3DRender::QLineWidth(this);
		lineWidth->setValue(3);
		pass->addRenderState(lineWidth);

		m_effect->addRenderPass(pass);
		setEffect(m_effect);
		// effect()->setParameter("offset", 0.1);
	}

	OutlineEntity::~OutlineEntity()
	{

	}

	void OutlineEntity::setRoute(std::vector<trimesh::vec3> route)
	{
		updateGeometry(route.size(), (float*)&route.at(0), true);
	}

	void OutlineEntity::setRoute(std::vector<std::vector<trimesh::vec3>> routes)
	{
		// setFloatOffset(routes);

		std::vector<trimesh::vec3> mergeRoute;
		for (auto r : routes)
		{
			for (auto v : r)
			{
				mergeRoute.push_back(v);
			}
		}

		updateGeometry(mergeRoute.size(), (float*)&mergeRoute.at(0), false);
	}

	void OutlineEntity::setOffset(float offset)
	{
		m_effect->setParameter("offset", offset);
	}
	// trimesh::vec3 OutlineEntity::triangleNormal(const trimesh::vec3& v1, const trimesh::vec3& v2, const trimesh::vec3& v3)
	// {
	// 	trimesh::vec3 v01 = v2 - v1;
	// 	trimesh::vec3 v02 = v3 - v1;
	// 	trimesh::vec3 n = v01 TRICROSS v02;
	// 	trimesh::normalize(n);
	// 	return n;
	// }

	// void OutlineEntity::setFloatOffset(std::vector<std::vector<trimesh::vec3>>& routes)
	// {
	// 	std::vector<trimesh::vec3> routeOffsets;
	// 	trimesh::vec3 tempOffset;
	// 	bool hasOffset = false;

	// 	for (int i = 0, rEnd = routes.size(); i < rEnd; ++i)
	// 	{
	// 		hasOffset = false;
	// 		tempOffset = trimesh::vec3();
	// 		/*auto r = routes[i];
	// 		int vCount = r.size();
	// 		if (!hasOffset && vCount >= 3)
	// 		{
	// 			tempOffset = triangleNormal(r[0], r[1], r[3]) * 1;
	// 			routeOffsets.push_back(tempOffset);
	// 			hasOffset = true;
	// 		}*/

	// 		for (; i < rEnd - 1; ++i)
	// 		{
	// 			auto cr = routes[i];
	// 			auto nr = routes[i + 1];

	// 			if (cr[cr.size() - 1] != nr[0])	// 两段路径不相连
	// 				break;

	// 			if (hasOffset)
	// 			{
	// 				routeOffsets.push_back(tempOffset);
	// 			}
	// 			else 
	// 			{
	// 				tempOffset = triangleNormal(cr[0], cr[cr.size() - 1], nr[nr.size() - 1]) * 4;
	// 				routeOffsets.push_back(tempOffset);
	// 				routeOffsets.push_back(tempOffset);
	// 				hasOffset = true;
	// 			}
	// 		}

	// 		if (!hasOffset)
	// 		{
	// 			routeOffsets.push_back(tempOffset);
	// 		}
	// 	}

	// 	// 添加偏移量
	// 	for (int i = 0, rCount = routes.size(); i < rCount; ++i)
	// 	{
	// 		auto r = routes[i];
	// 		auto offset = routeOffsets[i];
	// 		for (int j = 0, vCount = r.size(); j < vCount; ++j)
	// 		{
	// 			r[j] -= offset;
	// 		}
	// 	}
	// }
	
	void OutlineEntity::updateGeometry(int pointsNum, float* positions, bool loop)
	{
		Qt3DRender::QGeometryRenderer::PrimitiveType type = Qt3DRender::QGeometryRenderer::Lines;
		if (loop) type = Qt3DRender::QGeometryRenderer::LineLoop;
		setGeometry(LineCreateHelper::create(pointsNum, positions, NULL), type);
	}
}
