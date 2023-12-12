#include "newboxentity.h"

#include "qtuser3d/geometry/boxcreatehelper.h"
namespace qtuser_3d
{
	NewBoxEntity::NewBoxEntity(Qt3DCore::QNode* parent)
		:LineEntity(parent)
	{

	}

	NewBoxEntity::~NewBoxEntity()
	{

	}


	void NewBoxEntity::updateBox(Box3D& box)
	{
		QVector3D bmin = box.min;
		QVector3D bmax = box.max;
		QVector<QVector3D> m_positions;
		QVector<QVector4D> m_colors;
		m_positions.resize(24);
		m_colors.resize(24);

		m_positions[0] = bmin;
		m_positions[1] = QVector3D(bmax.x(), bmin.y(), bmin.z());

		m_positions[2] = QVector3D(bmax.x(), bmin.y(), bmin.z());
		m_positions[3] = QVector3D(bmax.x(), bmax.y(), bmin.z());

		m_positions[4] = QVector3D(bmax.x(), bmax.y(), bmin.z());
		m_positions[5] = QVector3D(bmin.x(), bmax.y(), bmin.z());

		m_positions[6] = QVector3D(bmin.x(), bmax.y(), bmin.z());
		m_positions[7] = QVector3D(bmin.x(), bmin.y(), bmin.z());

		m_positions[8] = QVector3D(bmin.x(), bmin.y(), bmin.z());
		m_positions[9] = QVector3D(bmin.x(), bmin.y(), bmax.z());

		m_positions[10] = QVector3D(bmax.x(), bmin.y(), bmin.z());
		m_positions[11] = QVector3D(bmax.x(), bmin.y(), bmax.z());

		m_positions[12] = QVector3D(bmax.x(), bmax.y(), bmin.z());
		m_positions[13] = QVector3D(bmax.x(), bmax.y(), bmax.z());

		m_positions[14] = QVector3D(bmin.x(), bmax.y(), bmin.z());
		m_positions[15] = QVector3D(bmin.x(), bmax.y(), bmax.z());
		//top
		m_positions[16] = QVector3D(bmin.x(), bmin.y(), bmax.z());;
		m_positions[17] = QVector3D(bmax.x(), bmin.y(), bmax.z());

		m_positions[18] = QVector3D(bmax.x(), bmin.y(), bmax.z());
		m_positions[19] = QVector3D(bmax.x(), bmax.y(), bmax.z());

		m_positions[20] = QVector3D(bmax.x(), bmax.y(), bmax.z());
		m_positions[21] = QVector3D(bmin.x(), bmax.y(), bmax.z());

		m_positions[22] = QVector3D(bmin.x(), bmax.y(), bmax.z());
		m_positions[23] = QVector3D(bmin.x(), bmin.y(), bmax.z());

		m_colors[0] = QVector4D(0.32f, 0.32f, 0.32f, 1.0);
		m_colors[1] = QVector4D(0.32f, 0.32f, 0.32f, 1.0);
		m_colors[2] = QVector4D(0.32f, 0.32f, 0.32f, 1.0);
		m_colors[3] = QVector4D(0.32f, 0.32f, 0.32f, 1.0);
		m_colors[4] = QVector4D(0.32f, 0.32f, 0.32f, 1.0);
		m_colors[5] = QVector4D(0.32f, 0.32f, 0.32f, 1.0);
		m_colors[6] = QVector4D(0.32f, 0.32f, 0.32f, 1.0);
		m_colors[7] = QVector4D(0.32f, 0.32f, 0.32f, 1.0);
		m_colors[8] = QVector4D(0.32f, 0.32f, 0.32f, 1.0);
		m_colors[9] = QVector4D(0.32f, 0.32f, 0.32f, 1.0);
		m_colors[10] = QVector4D(0.32f, 0.32f, 0.32f, 1.0);
		m_colors[11] = QVector4D(0.32f, 0.32f, 0.32f, 1.0);
		m_colors[12] = QVector4D(0.32f, 0.32f, 0.32f, 1.0);
		m_colors[13] = QVector4D(0.32f, 0.32f, 0.32f, 1.0);
		m_colors[14] = QVector4D(0.32f, 0.32f, 0.32f, 1.0);
		m_colors[15] = QVector4D(0.32f, 0.32f, 0.32f, 1.0);
		//top
		m_colors[16] = QVector4D(0.0f, 0.63f, 0.90f, 1.0);
		m_colors[17] = QVector4D(0.0f, 0.63f, 0.90f, 1.0);
		m_colors[18] = QVector4D(0.0f, 0.63f, 0.90f, 1.0);
		m_colors[19] = QVector4D(0.0f, 0.63f, 0.90f, 1.0);
		m_colors[20] = QVector4D(0.0f, 0.63f, 0.90f, 1.0);
		m_colors[21] = QVector4D(0.0f, 0.63f, 0.90f, 1.0);
		m_colors[22] = QVector4D(0.0f, 0.63f, 0.90f, 1.0);
		m_colors[23] = QVector4D(0.0f, 0.63f, 0.90f, 1.0);
		updateGeometry(m_positions, m_colors);
	}


	void NewBoxEntity::updateLocal(Box3D& box, const QMatrix4x4& parentMatrix)
	{
		setGeometry(BoxCreateHelper::createPartBox(box, 0.3f), Qt3DRender::QGeometryRenderer::Lines);

		QMatrix4x4 m = parentMatrix.inverted();
		setPose(m);
	}

	void NewBoxEntity::setColor(const QVector4D& color)
	{
		m_colorParameter->setValue(color);
	}
}
