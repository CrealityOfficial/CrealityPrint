#include "boxcreatehelper.h"

#include "qtuser3d/geometry/bufferhelper.h"
#include "qtuser3d/geometry/boxgeometrydata.h"

namespace qtuser_3d
{
	BoxCreateHelper::BoxCreateHelper(QObject* parent)
		:GeometryCreateHelper(parent)
	{
	}
	
	BoxCreateHelper::~BoxCreateHelper()
	{
	}

	Qt3DRender::QGeometry* BoxCreateHelper::create(Qt3DCore::QNode* parent)
	{
		Qt3DRender::QAttribute* positionAttribute = BufferHelper::CreateVertexAttribute((const char*)static_box_position, AttribueSlot::Position, static_box_vertex_count);
		Qt3DRender::QAttribute* indexAttribute = BufferHelper::CreateIndexAttribute((const char*)static_box_indices, 24);

		return GeometryCreateHelper::create(parent, positionAttribute, indexAttribute);
	}

	Qt3DRender::QGeometry* BoxCreateHelper::createNoBottom(Qt3DCore::QNode* parent)
	{
		Qt3DRender::QAttribute* positionAttribute = BufferHelper::CreateVertexAttribute((const char*)static_box_position, AttribueSlot::Position, static_box_vertex_count);
		Qt3DRender::QAttribute* indexAttribute = BufferHelper::CreateIndexAttribute((const char*)static_box_indices_nobottom, 16);

		return GeometryCreateHelper::create(parent, positionAttribute, indexAttribute);
	}

	void BoxCreateHelper::genDatasFromCorner(QVector<QVector3D>& positions, int pos, QVector3D boxsz, QVector3D dir)
	{
		positions[pos + 1 + 0] = positions[pos] + QVector3D(boxsz.x() * dir.x(), 0, 0);
		positions[pos + 1 + 1] = positions[pos] + QVector3D(0, boxsz.y() * dir.y(), 0);
		positions[pos + 1 + 2] = positions[pos] + QVector3D(0, 0, boxsz.z() * dir.z());
	}

	Qt3DRender::QGeometry* BoxCreateHelper::createPartBox(const Box3D& box, float percent, Qt3DCore::QNode* parent)
	{
		QVector<QVector3D> positions;
		positions.resize(32);

		QVector3D bmin = box.min;
		QVector3D bmax = box.max;

		QVector3D boxsz = (bmax - bmin) * (percent / 2);

		positions[0] = bmin;
		genDatasFromCorner(positions, 0, boxsz, QVector3D(1, 1, 1));

		positions[4] = QVector3D(bmax.x(), bmin.y(), bmin.z());
		genDatasFromCorner(positions, 4, boxsz, QVector3D(-1, 1, 1));

		positions[8] = QVector3D(bmax.x(), bmax.y(), bmin.z());
		genDatasFromCorner(positions, 8, boxsz, QVector3D(-1, -1, 1));

		positions[12] = QVector3D(bmin.x(), bmax.y(), bmin.z());
		genDatasFromCorner(positions, 12, boxsz, QVector3D(1, -1, 1));

		positions[16] = QVector3D(bmin.x(), bmin.y(), bmax.z());
		genDatasFromCorner(positions, 16, boxsz, QVector3D(1, 1, -1));

		positions[20] = QVector3D(bmax.x(), bmin.y(), bmax.z());
		genDatasFromCorner(positions, 20, boxsz, QVector3D(-1, 1, -1));

		positions[24] = bmax;
		genDatasFromCorner(positions, 24, boxsz, QVector3D(-1, -1, -1));

		positions[28] = QVector3D(bmin.x(), bmax.y(), bmax.z());
		genDatasFromCorner(positions, 28, boxsz, QVector3D(1, -1, -1));

		Qt3DRender::QAttribute* positionAttribute = BufferHelper::CreateVertexAttribute((const char*)&positions.at(0), AttribueSlot::Position, 32);
		Qt3DRender::QAttribute* indexAttribute = BufferHelper::CreateIndexAttribute((const char*)static_part_indices, 48);

		return GeometryCreateHelper::create(parent, positionAttribute, indexAttribute);
	}

	Qt3DRender::QGeometry* BoxCreateHelper::create(const Box3D& box, Qt3DCore::QNode* parent)
	{
		QVector<QVector3D> positions;
		positions.resize(8);

		QVector3D bmin = box.min;
		QVector3D bmax = box.max;

		positions[0] = bmin;
		positions[1] = QVector3D(bmax.x(), bmin.y(), bmin.z());
		positions[2] = QVector3D(bmax.x(), bmax.y(), bmin.z());
		positions[3] = QVector3D(bmin.x(), bmax.y(), bmin.z());
		positions[4] = QVector3D(bmin.x(), bmin.y(), bmax.z());
		positions[5] = QVector3D(bmax.x(), bmin.y(), bmax.z());
		positions[6] = bmax;
		positions[7] = QVector3D(bmin.x(), bmax.y(), bmax.z());

		Qt3DRender::QAttribute* positionAttribute = BufferHelper::CreateVertexAttribute((const char*)&positions.at(0), AttribueSlot::Position, 8);
		Qt3DRender::QAttribute* indexAttribute = BufferHelper::CreateIndexAttribute((const char*)static_box_indices, 24);

		return GeometryCreateHelper::create(parent, positionAttribute, indexAttribute);
	}

	Qt3DRender::QGeometry* BoxCreateHelper::createWithTriangle(const Box3D& box, Qt3DCore::QNode* parent)
	{
		QVector<QVector3D> positions;
		positions.resize(8);

		QVector3D bmin = box.min;
		QVector3D bmax = box.max;

		positions[0] = bmin;
		positions[1] = QVector3D(bmax.x(), bmin.y(), bmin.z());
		positions[2] = QVector3D(bmax.x(), bmax.y(), bmin.z());
		positions[3] = QVector3D(bmin.x(), bmax.y(), bmin.z());
		positions[4] = QVector3D(bmin.x(), bmin.y(), bmax.z());
		positions[5] = QVector3D(bmax.x(), bmin.y(), bmax.z());
		positions[6] = bmax;
		positions[7] = QVector3D(bmin.x(), bmax.y(), bmax.z());

		Qt3DRender::QAttribute* positionAttribute = BufferHelper::CreateVertexAttribute((const char*)&positions.at(0), AttribueSlot::Position, 8);
		Qt3DRender::QAttribute* indexAttribute = BufferHelper::CreateIndexAttribute((const char*)static_box_triangles_indices, 36);

		return GeometryCreateHelper::create(parent, positionAttribute, indexAttribute);
	}
}