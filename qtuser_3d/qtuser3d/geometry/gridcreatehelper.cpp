#include "gridcreatehelper.h"
#include "qtuser3d/geometry/bufferhelper.h"
#include <QtCore/qmath.h>
#include <QtGui/QVector2D>
#include <QtGui/QVector3D>

namespace qtuser_3d
{
	GridCreateHelper::GridCreateHelper(QObject* parent)
		:GeometryCreateHelper(parent)
	{
	}

	GridCreateHelper::~GridCreateHelper()
	{
	}

	Qt3DRender::QGeometry* GridCreateHelper::create(Box3D& box, float gap, Qt3DCore::QNode* parent)
	{
		QVector3D size = box.size();
		if (size.x() == 0 || size.y() == 0) return nullptr;

		float minX = box.min.x();
		float maxX = box.max.x();
		float minY = box.min.y();
		float maxY = box.max.y();

		int startX;
		if (minX == 0.0)
		{
			startX = 1;
		}
		else
		{
			startX = minX / gap;
		}
		int endX = qFloor(maxX / gap);
		if (float(endX) * gap == maxX) endX -= 1;

		int startY;
		if (minX == 0.0)
		{
			startY = 1;
		}
		else
		{
			startY = minY / gap;
		}
		int endY = qFloor(maxY / gap);
		if (float(endY) * gap == maxY) endY -= 1;

		int sizeX = endX - startX + 1;
		int sizeY = endY - startY + 1;
		if (sizeX <= 0 || sizeY <= 0)
			return nullptr;

		int vertexCount = 2 * (sizeX + sizeY);
		std::vector<QVector3D> positions(vertexCount);
		std::vector<QVector2D> flags(vertexCount);

		float z = 0.0f;

		int vertexIndex = 0;
		for (int i = startX; i <= endX; ++i)
		{
			positions.at(vertexIndex) = QVector3D(float(i) * gap, minY, z);
			flags.at(vertexIndex) = QVector2D(-1.0f, float(i) * gap);
			vertexIndex++;
			positions.at(vertexIndex) = QVector3D(float(i) * gap, maxY, z);
			flags.at(vertexIndex) = QVector2D(-1.0f, float(i) * gap);
			vertexIndex++;
		}
		for (int i = startY; i <= endY; ++i)
		{
			positions.at(vertexIndex) = QVector3D(minX, float(i) * gap, z);
			flags.at(vertexIndex) = QVector2D(1.0f, float(i) * gap);
			vertexIndex++;
			positions.at(vertexIndex) = QVector3D(maxX, float(i) * gap, z);
			flags.at(vertexIndex) = QVector2D(1.0f, float(i) * gap);
			vertexIndex++;
		}

		Qt3DRender::QAttribute* positionAttribute = BufferHelper::CreateVertexAttribute((const char*)&positions.at(0), AttribueSlot::Position, vertexCount);
		Qt3DRender::QAttribute* flagAttribute = BufferHelper::CreateVertexAttribute("vertexFlag", (const char*)&flags.at(0), 2, vertexCount);
		return GeometryCreateHelper::create(parent, positionAttribute, flagAttribute);
	}

	Qt3DRender::QGeometry* GridCreateHelper::createMid(Box3D& box, float gap, float offset, Qt3DCore::QNode* parent)
	{
		QVector3D size = box.size();
		if (size.x() == 0 || size.y() == 0) return nullptr;

		float minX = box.min.x();
		float maxX = box.max.x();
		float minY = box.min.y();
		float maxY = box.max.y();

		if (maxX - minX <= 0 || maxY - minY <= 0)
		{
			return nullptr;
		}
		std::vector<QVector3D> positions;
		std::vector<QVector2D> flags;

		float zcoord = 0.00f;
		float midX = (minX + maxX) / 2.0;
		float midY = (minY + maxY) / 2.0;

		int vertexCount = 0;

		float k = 0;
		for (float i = midX; i <= maxX; i += gap)
		{
			positions.push_back(QVector3D(i, minY, zcoord));
			positions.push_back(QVector3D(i, maxY, zcoord));

			flags.push_back(QVector2D(k, 1.0));
			flags.push_back(QVector2D(k, 1.0));

			k += gap;
			vertexCount += 2;
		}
		k = gap;
		for (float i = midX - gap; i >= minX; i -= gap)
		{
			positions.push_back(QVector3D(i, minY, zcoord));
			positions.push_back(QVector3D(i, maxY, zcoord));

			flags.push_back(QVector2D(k, 1.0));
			flags.push_back(QVector2D(k, 1.0));

			k += gap;
			vertexCount += 2;
		}

		k = 0;
		for (float i = midY; i <= maxY; i += gap)
		{
			positions.push_back(QVector3D(minX, i, zcoord));
			positions.push_back(QVector3D(maxX, i, zcoord));

			flags.push_back(QVector2D(1.0f, k));
			flags.push_back(QVector2D(1.0f, k));

			k += gap;
			vertexCount += 2;
		}
		k = gap;
		for (float i = midY - gap; i >= minY; i -= gap)
		{
			positions.push_back(QVector3D(minX, i, zcoord));
			positions.push_back(QVector3D(maxX, i, zcoord));

			flags.push_back(QVector2D(1.0f, k));
			flags.push_back(QVector2D(1.0f, k));

			k += gap;
			vertexCount += 2;
		}

		Qt3DRender::QAttribute* positionAttribute = BufferHelper::CreateVertexAttribute((const char*)&positions.at(0), AttribueSlot::Position, vertexCount);
		Qt3DRender::QAttribute* flagAttribute = BufferHelper::CreateVertexAttribute("vertexFlag", (const char*)&flags.at(0), 2, vertexCount);
		return GeometryCreateHelper::create(parent, positionAttribute, flagAttribute);
	}

	Qt3DRender::QGeometry* GridCreateHelper::createPlane(float width, float height, bool triangle, Qt3DCore::QNode* parent)
	{
		float w = qAbs(width);
		float h = qAbs(height);
		std::vector<QVector3D> positions;
		if (triangle)
		{
			positions.push_back(QVector3D(-w, -h, 0.0f));
			positions.push_back(QVector3D(w, -h, 0.0f));
			positions.push_back(QVector3D(-w, h, 0.0f));
			positions.push_back(QVector3D(w, -h, 0.0f));
			positions.push_back(QVector3D(w, h, 0.0f));
			positions.push_back(QVector3D(-w, h, 0.0f));
		}
		else
		{
			positions.push_back(QVector3D(-w, -h, 0.0f));
			positions.push_back(QVector3D(w, -h, 0.0f));
			positions.push_back(QVector3D(w, -h, 0.0f));
			positions.push_back(QVector3D(w, h, 0.0f));
			positions.push_back(QVector3D(w, h, 0.0f));
			positions.push_back(QVector3D(-w, h, 0.0f));
			positions.push_back(QVector3D(-w, h, 0.0f));
			positions.push_back(QVector3D(-w, -h, 0.0f));
		}

		Qt3DRender::QAttribute* positionAttribute = BufferHelper::CreateVertexAttribute((const char*)&positions.at(0), AttribueSlot::Position, (int)positions.size());
		return GeometryCreateHelper::create(parent, positionAttribute);
	}

	Qt3DRender::QGeometry* GridCreateHelper::createTextureQuad(Box3D& box, bool flatZ, Qt3DCore::QNode* parent)
	{
		QVector<QVector3D> points;
		QVector3D bmin = box.min;
		bmin.setZ(0.0f);
		QVector3D bmax = box.max;
		bmax.setZ(0.0f);
		points.push_back(bmin);
		points.push_back(QVector3D(bmax.x(), bmin.y(), 0.0f));
		points.push_back(bmax);
		points.push_back(bmin);
		points.push_back(bmax);
		points.push_back(QVector3D(bmin.x(), bmax.y(), 0.0f));

		QVector<QVector2D> texcoord;
		QVector2D tmin(0.0f, 0.0f);
		QVector2D tmax(1.0f, 1.0f);
		texcoord.push_back(tmin);
		texcoord.push_back(QVector2D(tmax.x(), tmin.y()));
		texcoord.push_back(tmax);
		texcoord.push_back(tmin);
		texcoord.push_back(tmax);
		texcoord.push_back(QVector2D(tmin.x(), tmax.y()));

		Qt3DRender::QAttribute* positionAttribute = qtuser_3d::BufferHelper::CreateVertexAttribute((const char*)(&points.at(0)), qtuser_3d::AttribueSlot::Position, points.size());
		Qt3DRender::QAttribute* texcoordAttribute = qtuser_3d::BufferHelper::CreateVertexAttribute("vertexTexCoord", (const char*)(&texcoord.at(0)), 2, points.size());

		return GeometryCreateHelper::create(parent, positionAttribute, texcoordAttribute);;
	}

}