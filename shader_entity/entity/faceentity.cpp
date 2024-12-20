#include "faceentity.h"
#include "qtuser3d/geometry/trianglescreatehelper.h"
#include "faces.h"

namespace qtuser_3d
{
	FaceEntity::FaceEntity(Qt3DCore::QNode* parent)
		: XEntity(parent)
	{
		m_faceFront = new Faces(this);
		m_faceBack = new Faces(this);
		m_faceLeft = new Faces(this);
		m_faceRight = new Faces(this);
		m_faceTop = new Faces(this);
		m_faceBottom = new Faces(this);

	}

	FaceEntity::~FaceEntity()
	{
	}

	void FaceEntity::drawFace(Box3D& box)
	{
		int vertexNum = 8;
		int triangleNum = 12;
		QVector<QVector3D> positions;
		QVector<unsigned> indices;

		float minX = box.min.x();
		float maxX = box.max.x();
		float minY = box.min.y();
		float maxY = box.max.y();
		float minZ = box.min.z();
		float maxZ = box.max.z();


		positions.push_back(QVector3D(minX, minY, minZ));
		positions.push_back(QVector3D(minX, minY, maxZ));
		positions.push_back(QVector3D(maxX, minY, maxZ));
		positions.push_back(QVector3D(maxX, minY, minZ));

		positions.push_back(QVector3D(minX, maxY, minZ));
		positions.push_back(QVector3D(minX, maxY, maxZ));
		positions.push_back(QVector3D(maxX, maxY, maxZ));
		positions.push_back(QVector3D(maxX, maxY, minZ));

		//front
		indices.push_back(0); indices.push_back(3); indices.push_back(2);
		indices.push_back(2); indices.push_back(1); indices.push_back(0);
		//right
		indices.push_back(3); indices.push_back(7); indices.push_back(6);
		indices.push_back(3); indices.push_back(6); indices.push_back(2);
		//back
		indices.push_back(4); indices.push_back(7); indices.push_back(6);
		indices.push_back(4); indices.push_back(6); indices.push_back(5);
		//left
		indices.push_back(5); indices.push_back(4); indices.push_back(1);
		indices.push_back(4); indices.push_back(0); indices.push_back(1);
		//up
		indices.push_back(1); indices.push_back(2); indices.push_back(6);
		indices.push_back(1); indices.push_back(6); indices.push_back(5);
		//down
		indices.push_back(0); indices.push_back(3); indices.push_back(7);
		indices.push_back(0); indices.push_back(7); indices.push_back(4);

		Qt3DRender::QGeometry* geometry = qtuser_3d::TrianglesCreateHelper::create(vertexNum, (float*)&positions.at(0), nullptr, nullptr, triangleNum, (unsigned*)&indices.at(0));
		setGeometry(geometry);
	}

	void FaceEntity::updateFace(Box3D& box, faceType type)
	{
		if (type == faceType::left && m_faceLeft)
		{
			int vertexNum = 4;
			int triangleNum = 2;
			QVector<QVector3D> positions;
			QVector<unsigned> indices;
			float minX = box.min.x();
			float minY = box.min.y();
			float maxY = box.max.y();
			float minZ = box.min.z();
			float maxZ = box.max.z();
			positions.push_back(QVector3D(minX, minY, minZ));
			positions.push_back(QVector3D(minX, minY, maxZ));
			positions.push_back(QVector3D(minX, maxY, minZ));
			positions.push_back(QVector3D(minX, maxY, maxZ));
			//front
			indices.push_back(0); indices.push_back(2); indices.push_back(3);
			indices.push_back(0); indices.push_back(3); indices.push_back(1);
			Qt3DRender::QGeometry* geometry = qtuser_3d::TrianglesCreateHelper::create(vertexNum, (float*)&positions.at(0), nullptr, nullptr, triangleNum, (unsigned*)&indices.at(0));
			m_faceLeft->setGeometry(geometry);
			setVisibility((int)faceType::left, true);
		}

		if (type == faceType::front && m_faceFront)
		{
			int vertexNum = 4;
			int triangleNum = 2;
			QVector<QVector3D> positions;
			QVector<unsigned> indices;
			float minX = box.min.x();
			float maxX = box.max.x();
			float minY = box.min.y();
			float minZ = box.min.z();
			float maxZ = box.max.z();
			positions.push_back(QVector3D(minX, minY, minZ));
			positions.push_back(QVector3D(minX, minY, maxZ));
			positions.push_back(QVector3D(maxX, minY, maxZ));
			positions.push_back(QVector3D(maxX, minY, minZ));
			//front
			indices.push_back(0); indices.push_back(3); indices.push_back(2);
			indices.push_back(2); indices.push_back(1); indices.push_back(0);
			Qt3DRender::QGeometry* geometry = qtuser_3d::TrianglesCreateHelper::create(vertexNum, (float*)&positions.at(0), nullptr, nullptr, triangleNum, (unsigned*)&indices.at(0));
			m_faceFront->setGeometry(geometry);
			setVisibility((int)faceType::front, true);
		}

		if (type == faceType::right && m_faceRight)
		{
			int vertexNum = 4;
			int triangleNum = 2;
			QVector<QVector3D> positions;
			QVector<unsigned> indices;

			float maxX = box.max.x();
			float minY = box.min.y();
			float maxY = box.max.y();
			float minZ = box.min.z();
			float maxZ = box.max.z();
			positions.push_back(QVector3D(maxX, minY, maxZ));
			positions.push_back(QVector3D(maxX, minY, minZ));
			positions.push_back(QVector3D(maxX, maxY, maxZ));
			positions.push_back(QVector3D(maxX, maxY, minZ));
			//right
			indices.push_back(1); indices.push_back(2); indices.push_back(0);
			indices.push_back(1); indices.push_back(3); indices.push_back(2);
			Qt3DRender::QGeometry* geometry = qtuser_3d::TrianglesCreateHelper::create(vertexNum, (float*)&positions.at(0), nullptr, nullptr, triangleNum, (unsigned*)&indices.at(0));
			m_faceRight->setGeometry(geometry);
			setVisibility((int)faceType::right, true);
		}

		if (type == faceType::back && m_faceBack)
		{
			int vertexNum = 4;
			int triangleNum = 2;
			QVector<QVector3D> positions;
			QVector<unsigned> indices;
			float minX = box.min.x();
			float maxX = box.max.x();
			float maxY = box.max.y();
			float minZ = box.min.z();
			float maxZ = box.max.z();
			positions.push_back(QVector3D(minX, maxY, minZ));
			positions.push_back(QVector3D(minX, maxY, maxZ));
			positions.push_back(QVector3D(maxX, maxY, maxZ));
			positions.push_back(QVector3D(maxX, maxY, minZ));
			//back
			indices.push_back(0); indices.push_back(3); indices.push_back(2);
			indices.push_back(0); indices.push_back(2); indices.push_back(1);
			Qt3DRender::QGeometry* geometry = qtuser_3d::TrianglesCreateHelper::create(vertexNum, (float*)&positions.at(0), nullptr, nullptr, triangleNum, (unsigned*)&indices.at(0));
			m_faceBack->setGeometry(geometry);
			setVisibility((int)faceType::back, true);
		}

		if (type == faceType::up && m_faceTop)
		{
			int vertexNum = 4;
			int triangleNum = 2;
			QVector<QVector3D> positions;
			QVector<unsigned> indices;
			float minX = box.min.x();
			float maxX = box.max.x();
			float minY = box.min.y();
			float maxY = box.max.y();
			float maxZ = box.max.z();
			positions.push_back(QVector3D(minX, minY, maxZ));
			positions.push_back(QVector3D(maxX, minY, maxZ));
			positions.push_back(QVector3D(minX, maxY, maxZ));
			positions.push_back(QVector3D(maxX, maxY, maxZ));
			//up
			indices.push_back(0); indices.push_back(1); indices.push_back(3);
			indices.push_back(0); indices.push_back(3); indices.push_back(2);
			Qt3DRender::QGeometry* geometry = qtuser_3d::TrianglesCreateHelper::create(vertexNum, (float*)&positions.at(0), nullptr, nullptr, triangleNum, (unsigned*)&indices.at(0));
			m_faceTop->setGeometry(geometry);
			setVisibility((int)faceType::up, true);
		}

		if (type == faceType::down && m_faceBottom)
		{
			int vertexNum = 4;
			int triangleNum = 2;
			QVector<QVector3D> positions;
			QVector<unsigned> indices;
			float minX = box.min.x();
			float maxX = box.max.x();
			float minY = box.min.y();
			float maxY = box.max.y();
			float minZ = box.min.z();
			positions.push_back(QVector3D(minX, minY, minZ));
			positions.push_back(QVector3D(maxX, minY, minZ));
			positions.push_back(QVector3D(minX, maxY, minZ));
			positions.push_back(QVector3D(maxX, maxY, minZ));
			//down
			indices.push_back(0); indices.push_back(1); indices.push_back(3);
			indices.push_back(0); indices.push_back(3); indices.push_back(2);
			Qt3DRender::QGeometry* geometry = qtuser_3d::TrianglesCreateHelper::create(vertexNum, (float*)&positions.at(0), nullptr, nullptr, triangleNum, (unsigned*)&indices.at(0));
			m_faceBottom->setGeometry(geometry);
			setVisibility((int)faceType::down, true);
		}
	}

	void FaceEntity::setColor(int type, const QVector4D& color)
	{
		if (type == faceType::back && m_faceBack)
		{
			m_faceBack->setColor(color);
		}
		else if (type == faceType::left && m_faceLeft)
		{
			m_faceLeft->setColor(color);
		}
		else if (type == faceType::right && m_faceRight)
		{
			m_faceRight->setColor(color);
		}
		else if (type == faceType::front && m_faceFront)
		{
			m_faceFront->setColor(color);
		}
		else if (type == faceType::up && m_faceTop)
		{
			m_faceTop->setColor(color);
		}
		else if (type == faceType::down && m_faceBottom)
		{
			m_faceBottom->setColor(color);
		}
	}

	void FaceEntity::setVisibility(int type, bool visibility)
	{
		if (type == faceType::back && m_faceBack)
		{
			m_faceBack->setEnabled(visibility ? true : false);
		}
		if (type == faceType::left && m_faceLeft)
		{
			m_faceLeft->setEnabled(visibility ? true : false);
		}
		if (type == faceType::right && m_faceRight)
		{
			m_faceRight->setEnabled(visibility ? true : false);
		}
		if (type == faceType::front && m_faceFront)
		{
			m_faceFront->setEnabled(visibility ? true : false);
		}
		if (type == faceType::up && m_faceTop)
		{
			m_faceTop->setEnabled(visibility ? true : false);
		}
		if (type == faceType::down && m_faceBottom)
		{
			m_faceBottom->setEnabled(visibility ? true : false);
		}
	}


}