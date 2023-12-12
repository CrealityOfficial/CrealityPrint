#include "trianglescreatehelper.h"
#include "qtuser3d/geometry/bufferhelper.h"

#include <QtGui/QVector3D>

namespace qtuser_3d
{
	TrianglesCreateHelper::TrianglesCreateHelper(QObject* parent)
		:GeometryCreateHelper(parent)
	{
	}
	
	TrianglesCreateHelper::~TrianglesCreateHelper()
	{
	}

	Qt3DRender::QGeometry* TrianglesCreateHelper::create(int vertexNum, float* position, float* normals, float* colors, int triangleNum, unsigned* indices
		, Qt3DCore::QNode* parent)
	{
		Qt3DRender::QAttribute* positionAttribute = nullptr;
		Qt3DRender::QAttribute* normalAttribute = nullptr;
		Qt3DRender::QAttribute* colorAttribute = nullptr;
		Qt3DRender::QAttribute* indexAttribute = nullptr;

		if (position)
			positionAttribute = BufferHelper::CreateVertexAttribute((const char*)position, AttribueSlot::Position, vertexNum);

		if (normals)
			normalAttribute = BufferHelper::CreateVertexAttribute((const char*)normals, AttribueSlot::Normal, vertexNum);

		if (colors)
			colorAttribute = BufferHelper::CreateVertexAttribute((const char*)colors, AttribueSlot::Color, vertexNum);

		if (indices)
			indexAttribute = BufferHelper::CreateIndexAttribute((const char*)indices, 3 * triangleNum);

		return GeometryCreateHelper::create(parent, positionAttribute, normalAttribute, colorAttribute, indexAttribute);
	}


	Qt3DRender::QGeometry* TrianglesCreateHelper::createWithTex(int vertexNum, float* position, float* normals, float* texcoords, int triangleNum, unsigned* indices
		, Qt3DCore::QNode* parent)
	{
		Qt3DRender::QAttribute* positionAttribute = nullptr;
		Qt3DRender::QAttribute* normalAttribute = nullptr;
		Qt3DRender::QAttribute* texcoordAttribute = nullptr;
		Qt3DRender::QAttribute* indexAttribute = nullptr;

		if (position)
			positionAttribute = BufferHelper::CreateVertexAttribute((const char*)position, AttribueSlot::Position, vertexNum);

		if (normals)
			normalAttribute = BufferHelper::CreateVertexAttribute((const char*)normals, AttribueSlot::Normal, vertexNum);

		if (texcoords)
			texcoordAttribute = BufferHelper::CreateVertexAttribute((const char*)texcoords, AttribueSlot::Texcoord, vertexNum);

		if (indices)
			indexAttribute = BufferHelper::CreateIndexAttribute((const char*)indices, 3 * triangleNum);

		return GeometryCreateHelper::create(parent, positionAttribute, normalAttribute, texcoordAttribute, indexAttribute);
	}

	Qt3DRender::QGeometry* TrianglesCreateHelper::createFromVertex(int vertexNum, float* position, Qt3DCore::QNode* parent)
	{
		int triangleNum = vertexNum / 3;
		if (triangleNum <= 0)
			return nullptr;

		Qt3DRender::QAttribute* positionAttribute = BufferHelper::CreateVertexAttribute((const char*)position, AttribueSlot::Position, 3 * triangleNum);;
		QVector3D* positionData = (QVector3D*)position;
		std::vector<QVector3D> normalData(3 * triangleNum);
		for (int i = 0; i < triangleNum; ++i)
		{
			QVector3D* startPosition = positionData + 3 * i;

			QVector3D v1 = startPosition[1] - startPosition[0];
			QVector3D v2 = startPosition[2] - startPosition[0];
			QVector3D n = QVector3D::crossProduct(v1, v2);
			n.normalize();
			normalData.at(3 * i) = n;
			normalData.at(3 * i + 1) = n;
			normalData.at(3 * i + 2) = n;
		}

		Qt3DRender::QAttribute* normalAttribute = BufferHelper::CreateVertexAttribute((const char*)&normalData.at(0), AttribueSlot::Normal, 3 * triangleNum);
		return GeometryCreateHelper::create(parent, positionAttribute, normalAttribute);
	}
}