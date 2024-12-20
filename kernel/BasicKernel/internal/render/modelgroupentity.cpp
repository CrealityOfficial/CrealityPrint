#include "modelgroupentity.h"
#include "qtuser3d/trimesh2/renderprimitive.h"
#include "qtuser3d/trimesh2/trimeshrender.h"
#include "qtuser3d/refactor/xeffect.h"
#include "entity/pureentity.h"
#include <QVector2D>
#include "interface/spaceinterface.h"
#include "interface/visualsceneinterface.h"
#include "entity/nozzleregionentity.h"
#include "interface/renderinterface.h"

namespace creative_kernel
{
	ModelGroupEntity::ModelGroupEntity(Qt3DCore::QNode* parent)
		:XEntity(parent)
	{
		m_convexEntity = new qtuser_3d::PureEntity(spaceEntity());
		m_convexEntity->addPassFilter(0, "view");
		m_convexEntity->setColor(QVector4D(1.0f, 0.0f, 0.0f, 1.0f));

		m_lineEntity = new qtuser_3d::PureEntity();
		qtuser_3d::XRenderPass* pass = m_lineEntity->xeffect()->findRenderPass(0);
		if (pass)
		{
			pass->addFilterKeyMask("alpha2nd", 0);
			pass->setParameter("color", QVector4D(0.5f, 0.5f, 0.5f, 1.0f));
			pass->setLineWidth(3.0f);
			pass->setPassDepthTest(Qt3DRender::QDepthTest::LessOrEqual);
			pass->setPassNoDepthMask();
		}
		setOuterLinesVisibility(false);

		m_nozzleRegionEntity = new qtuser_3d::NozzleRegionEntity(spaceEntity());
		setNozzleRegionVisibility(false);
		connect(this, &ModelGroupEntity::enabledChanged, this, [=](bool enabled)
			{
				m_lineEntity->setEnabled(enabled && m_lineEntityVisible);
				m_nozzleRegionEntity->setEnabled(enabled && m_nozzleRegionVisible);
			});
	}
	
	ModelGroupEntity::~ModelGroupEntity()
	{
		if (m_convexEntity)
		{
			delete m_convexEntity;
			m_convexEntity = nullptr;
		}

		if (m_lineEntity)
		{
			m_lineEntity->setParent((Qt3DCore::QNode*)nullptr);
			delete m_lineEntity;
		}
		
		if (m_nozzleRegionEntity)
		{
			m_nozzleRegionEntity->setParent((Qt3DCore::QNode*)nullptr);
			delete m_nozzleRegionEntity;
		}
	}

	void ModelGroupEntity::updateConvex(const std::vector<trimesh::vec3>& lines)
	{
		m_convexEntity->setGeometry(qtuser_3d::createLinesGeometry(lines), Qt3DRender::QGeometryRenderer::Lines);
	}

	void ModelGroupEntity::setConvexPose(const QMatrix4x4& matrix)
	{
		m_convexEntity->setPose(matrix);
	}

	void ModelGroupEntity::setLinesPose(const QMatrix4x4& pose)
	{
		m_lineEntity->setPose(pose);
		m_nozzleRegionEntity->setPose(pose);
	}

	void ModelGroupEntity::updateLines(const std::vector<trimesh::vec3>& lines)
	{
		if (lines.size() <= 0)
			return;

		requestRender(this);
		m_lineEntity->setEnabled(false);
		m_nozzleRegionEntity->setEnabled(false);

		std::vector<trimesh::vec3> rlines;
		qtuser_3d::loopPolygon2Lines(lines, rlines);
		m_lineEntity->setGeometry(qtuser_3d::createLinesGeometry(rlines), Qt3DRender::QGeometryRenderer::Lines);

		trimesh::box3 box;
		for (auto line : lines)
		{
			box += line;
		}

		trimesh::vec3 center = box.center();
		std::vector<trimesh::vec3> newLines;
		newLines.push_back(center);
		newLines.insert(newLines.cend(), lines.begin(), lines.end());
		newLines.push_back(lines.front());
		m_nozzleRegionEntity->setGeometry(qtuser_3d::createTriangles(newLines), Qt3DRender::QGeometryRenderer::TriangleFan);
	
		m_lineEntity->setEnabled(true && m_lineEntityVisible);
		m_nozzleRegionEntity->setEnabled(true && m_nozzleRegionVisible);
		endRequestRender(this);
	}

	void ModelGroupEntity::setOuterLinesColor(const QVector4D& color)
	{
		m_lineEntity->setColor(color);
	}

	void ModelGroupEntity::setOuterLinesVisibility(bool visible)
	{
		if (m_lineEntity->parent() != this->parent())
		{
			m_lineEntity->setParent((Qt3DCore::QNode*)this->parent());
		}
		if (m_lineEntity->isEnabled() != visible)
		{
			m_lineEntity->setEnabled(visible);
		}
		m_lineEntityVisible = visible;
	}

	void ModelGroupEntity::setNozzleRegionVisibility(bool visible)
	{
		if (m_nozzleRegionEntity->parent() != this->parent())
		{
			m_nozzleRegionEntity->setParent((Qt3DCore::QNode*)this->parent());
		}
		if (m_nozzleRegionEntity->isEnabled() != visible)
		{
			m_nozzleRegionEntity->setEnabled(visible);
		}
		m_nozzleRegionVisible = visible;
	}
}

