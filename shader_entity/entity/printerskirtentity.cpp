#include "printerskirtentity.h"
#include "qtuser3d/refactor/xrenderpass.h"
#include "qtuser3d/refactor/xeffect.h"
#include "qtuser3d/geometry/trianglescreatehelper.h"
#include "renderpass/purerenderpass.h"

namespace qtuser_3d
{
	PrinterSkirtDecorEntity::PrinterSkirtDecorEntity(Qt3DCore::QNode* parent)
		:XEntity(parent)
	{
		//QVector4D color(0.180f, 0.541f, 0.968f, 1.0f);
		QVector4D color(0.15f, 0.15f, 0.15f, 1.0f);
		m_colorParameter = setParameter("color", color);

		XRenderPass* pass = new PureRenderPass(this);
		pass->addFilterKeyMask("view", 0);
		pass->setPassDepthTest();

		XEffect* effect = new XEffect(this);
		effect->addRenderPass(pass);
		setEffect(effect);
	}
	
	PrinterSkirtDecorEntity::~PrinterSkirtDecorEntity()
	{
	}

	void PrinterSkirtDecorEntity::updateBoundingBox(const Box3D& box)
	{
		QVector<QVector3D> positions;
		QVector<unsigned> indices;

		float minX = box.min.x();
		float maxX = box.max.x();
		float minY = box.min.y();
		float maxY = box.max.y();

		float offset = 4 / 220.0 * (maxX - minX);
		//float textoffset = 14.0 / 220.0 * (maxX - minX);

		//float logoffset = (maxX - minX) / 8.0 * 3.0;
		//float cha = 2.0 / 220.0 * (maxX - minX);

		positions.push_back(QVector3D(minX, minY, 0.0f));
		positions.push_back(QVector3D(minX, minY, 0.0f) + QVector3D(-offset, -offset, 0.0f));

		//positions.push_back(QVector3D(minX + logoffset, minY, 0.0f));
		//positions.push_back(QVector3D(minX + logoffset, minY - offset, 0.0f));
		//positions.push_back(QVector3D(maxX - logoffset, minY, 0.0f));
		//positions.push_back(QVector3D(maxX - logoffset, minY - offset, 0.0f));

		positions.push_back(QVector3D(maxX, minY, 0.0f));
		positions.push_back(QVector3D(maxX, minY, 0.0f) + QVector3D(offset, -offset, 0.0f));
		positions.push_back(QVector3D(maxX, maxY, 0.0f));
		positions.push_back(QVector3D(maxX, maxY, 0.0f) + QVector3D(offset, offset, 0.0f));
		positions.push_back(QVector3D(minX, maxY, 0.0f));
		positions.push_back(QVector3D(minX, maxY, 0.0f) + QVector3D(-offset, offset, 0.0f));

		//positions.push_back(QVector3D(minX + logoffset, minY - cha, 0.0f));
		//positions.push_back(QVector3D(minX + logoffset, minY - offset + cha, 0.0f));
		//positions.push_back(QVector3D(maxX - logoffset, minY - cha, 0.0f));
		//positions.push_back(QVector3D(maxX - logoffset, minY - offset + cha, 0.0f));

		//positions.push_back(QVector3D(minX - offset, minY - textoffset, 0.0f));
		//positions.push_back(QVector3D(maxX + offset, minY - textoffset, 0.0f));
		//positions.push_back(QVector3D(maxX + offset, minY - offset, 0.0f));
		//positions.push_back(QVector3D(minX - offset, minY - offset, 0.0f));


		indices.push_back(0); indices.push_back(1); indices.push_back(2);
		indices.push_back(2); indices.push_back(1); indices.push_back(3);
		indices.push_back(2); indices.push_back(3); indices.push_back(4);
		indices.push_back(4); indices.push_back(3); indices.push_back(5);
		indices.push_back(4); indices.push_back(7); indices.push_back(6);
		indices.push_back(4); indices.push_back(5); indices.push_back(7);
		indices.push_back(6); indices.push_back(7); indices.push_back(1);
		indices.push_back(0); indices.push_back(6); indices.push_back(1);

		//indices.push_back(8); indices.push_back(9); indices.push_back(10);
		//indices.push_back(8); indices.push_back(10); indices.push_back(11);



		//indices.push_back(0); indices.push_back(1); indices.push_back(2);
		//indices.push_back(2); indices.push_back(1); indices.push_back(3);
		//indices.push_back(4); indices.push_back(5); indices.push_back(6);
		//indices.push_back(6); indices.push_back(5); indices.push_back(7);

		//indices.push_back(2 + 4); indices.push_back(3 + 4); indices.push_back(4 + 4);
		//indices.push_back(4 + 4); indices.push_back(3 + 4); indices.push_back(5 + 4);
		//indices.push_back(4 + 4); indices.push_back(7 + 4); indices.push_back(6 + 4);
		//indices.push_back(4 + 4); indices.push_back(5 + 4); indices.push_back(7 + 4);
		//indices.push_back(6 + 4); indices.push_back(7 + 4); indices.push_back(1);
		//indices.push_back(0); indices.push_back(6 + 4); indices.push_back(1);

		//indices.push_back(2); indices.push_back(12); indices.push_back(4);
		//indices.push_back(4); indices.push_back(12); indices.push_back(14);
		//indices.push_back(13); indices.push_back(3); indices.push_back(15);
		//indices.push_back(15); indices.push_back(3); indices.push_back(5);

		Qt3DRender::QGeometry* geometry = qtuser_3d::TrianglesCreateHelper::create(positions.size(), (float*)&positions.at(0), nullptr, nullptr, indices.size() / 3, (unsigned*)&indices.at(0));
		setGeometry(geometry);
	}

	void PrinterSkirtDecorEntity::setColor(const QVector4D& color)
	{
		m_colorParameter->setValue(color);
	}



	PrinterSkirtEntity::PrinterSkirtEntity(Qt3DCore::QNode* parent)
		:XEntity(parent)
	{
		m_outerEntity = new PrinterSkirtDecorEntity(this);
		m_verticalEntity = new PrinterSkirtDecorEntity(this);
		m_innerHighlightEntity = new PrinterSkirtDecorEntity(this);
		//m_innerHighlightEntity->setPassDepthTest("pure", Qt3DRender::QDepthTest::LessOrEqual);
	}

	PrinterSkirtEntity::~PrinterSkirtEntity()
	{
	}

	void PrinterSkirtEntity::updateBoundingBox(const Box3D& box)
	{
		float minX = box.min.x();
		float maxX = box.max.x();
		float minY = box.min.y();
		float maxY = box.max.y();
		float offset = 4 / 220.0 * (maxX - minX);

		{
			QVector<QVector3D> positions;
			QVector<unsigned> indices;

			positions.push_back(QVector3D(minX, minY, 0.0f));
			positions.push_back(QVector3D(minX, minY, 0.0f) + QVector3D(-offset, -offset, 0.0f));
			positions.push_back(QVector3D(maxX, minY, 0.0f));
			positions.push_back(QVector3D(maxX, minY, 0.0f) + QVector3D(offset, -offset, 0.0f));
			positions.push_back(QVector3D(maxX, maxY, 0.0f));
			positions.push_back(QVector3D(maxX, maxY, 0.0f) + QVector3D(offset, offset, 0.0f));
			positions.push_back(QVector3D(minX, maxY, 0.0f));
			positions.push_back(QVector3D(minX, maxY, 0.0f) + QVector3D(-offset, offset, 0.0f));


			indices.push_back(0); indices.push_back(1); indices.push_back(2);
			indices.push_back(2); indices.push_back(1); indices.push_back(3);
			indices.push_back(2); indices.push_back(3); indices.push_back(4);
			indices.push_back(4); indices.push_back(3); indices.push_back(5);
			indices.push_back(4); indices.push_back(7); indices.push_back(6);
			indices.push_back(4); indices.push_back(5); indices.push_back(7);
			indices.push_back(6); indices.push_back(7); indices.push_back(1);
			indices.push_back(0); indices.push_back(6); indices.push_back(1);


			Qt3DRender::QGeometry* geometry = qtuser_3d::TrianglesCreateHelper::create(positions.size(), (float*)&positions.at(0), nullptr, nullptr, indices.size() / 3, (unsigned*)&indices.at(0));
			m_outerEntity->setGeometry(geometry);
		}

		{
			QVector3D verticalOffset(0.0, 0.0, offset);

			QVector3D bottomLeft = QVector3D(minX, minY, 0.0f) + QVector3D(-offset, -offset, 0.0f);
			QVector3D bottomLeftB = bottomLeft - verticalOffset;

			QVector3D bottomRight = QVector3D(maxX, minY, 0.0f) + QVector3D(offset, -offset, 0.0f);
			QVector3D bottomRightB = bottomRight - verticalOffset;

			QVector3D topRight = QVector3D(maxX, maxY, 0.0f) + QVector3D(offset, offset, 0.0f);
			QVector3D topRightB = topRight - verticalOffset;

			QVector3D topLeft = QVector3D(minX, maxY, 0.0f) + QVector3D(-offset, offset, 0.0f);
			QVector3D topLeftB = topLeft - verticalOffset;

			QVector<QVector3D> positions;
			QVector<unsigned> indices;

			positions.push_back(bottomLeft);
			positions.push_back(bottomLeftB);
			positions.push_back(bottomRight);
			positions.push_back(bottomRightB);
			positions.push_back(topRight);
			positions.push_back(topRightB);
			positions.push_back(topLeft);
			positions.push_back(topLeftB);


			indices.push_back(0); indices.push_back(1); indices.push_back(2);
			indices.push_back(2); indices.push_back(1); indices.push_back(3);

			indices.push_back(2); indices.push_back(3); indices.push_back(4);
			indices.push_back(4); indices.push_back(3); indices.push_back(5);

			indices.push_back(4); indices.push_back(5); indices.push_back(6);
			indices.push_back(6); indices.push_back(5); indices.push_back(7);

			indices.push_back(6); indices.push_back(7); indices.push_back(0);
			indices.push_back(0); indices.push_back(7); indices.push_back(1);


			Qt3DRender::QGeometry* geometry = qtuser_3d::TrianglesCreateHelper::create(positions.size(), (float*)&positions.at(0), nullptr, nullptr, indices.size() / 3, (unsigned*)&indices.at(0));
			m_verticalEntity->setGeometry(geometry);
		}

		{
			auto makeQuad = [](QVector3D A, QVector3D B, QVector3D C, QVector3D D, QVector<QVector3D>& result)->void {
				result.push_back(A);
				result.push_back(B);
				result.push_back(C);
				result.push_back(A);
				result.push_back(C);
				result.push_back(D);
			};

			float minZ = box.min.z() + 0.01;
			float gap = 2.0;
			QVector<QVector3D> pos;

			QVector3D pA = QVector3D(minX, minY, minZ);
			QVector3D pB = QVector3D(maxX, minY, minZ);
			QVector3D pC = QVector3D(maxX, maxY, minZ);
			QVector3D pD = QVector3D(minX, maxY, minZ);


			{
				QVector3D c = QVector3D(maxX, minY + gap, minZ);
				QVector3D d = QVector3D(minX, minY + gap, minZ);

				makeQuad(pA, pB, c, d, pos);
			}

			{
				QVector3D a = QVector3D(minX, maxY - gap, minZ);
				QVector3D b = QVector3D(maxX, maxY - gap, minZ);

				makeQuad(a, b, pC, pD, pos);
			}

			{
				QVector3D a = QVector3D(minX, minY + gap, minZ);
				QVector3D b = QVector3D(minX + gap, minY + gap, minZ);
				QVector3D c = QVector3D(minX + gap, maxY - gap, minZ);
				QVector3D d = QVector3D(minX, maxY - gap, minZ);

				makeQuad(a, b, c, d, pos);
			}

			{
				QVector3D a = QVector3D(maxX - gap, minY + gap, minZ);
				QVector3D b = QVector3D(maxX, minY + gap, minZ);
				QVector3D c = QVector3D(maxX, maxY - gap, minZ);
				QVector3D d = QVector3D(maxX - gap, maxY - gap, minZ);

				makeQuad(a, b, c, d, pos);
			}

			Qt3DRender::QGeometry* geo = TrianglesCreateHelper::createFromVertex(pos.size(), (float*)&pos.at(0), m_innerHighlightEntity);
			m_innerHighlightEntity->setGeometry(geo);
		}
	}

	void PrinterSkirtEntity::setInnerColor(const QVector4D& color)
	{
		m_innerHighlightEntity->setColor(color);
	}

	void PrinterSkirtEntity::setOuterColor(const QVector4D& color)
	{
		m_outerEntity->setColor(color);
	}

	void PrinterSkirtEntity::setVerticalBottomColor(const QVector4D& color)
	{
		m_verticalEntity->setColor(color);
	}

	void PrinterSkirtEntity::setHighlight(bool highlight)
	{
		m_innerHighlightEntity->setEnabled(highlight);
	}

}