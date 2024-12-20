#include "axisentity.h"
#include "qtuser3d/utils/primitiveshapecache.h"
#include "entity/purecolorentity.h"
#include "entity/textmeshentity.h"

namespace qtuser_3d
{
	AxisEntity::AxisEntity(Qt3DCore::QNode* parent, int axistype, QVector3D* s_use)
		:XEntity(parent)
	{
		setObjectName("AxisEntity");
		m_xAxis = new qtuser_3d::PureColorEntity(this);
		m_xAxis->setObjectName("AxisEntity.xAxis");
		
		m_yAxis = new qtuser_3d::PureColorEntity(this);
		m_yAxis->setObjectName("AxisEntity.yAxis");

		m_zAxis = new qtuser_3d::PureColorEntity(this);
		m_zAxis->setObjectName("AxisEntity.zAxis");
		
		Qt3DRender::QGeometry* geometry = nullptr;
		QVector3D s(50.0f, 50.0f, 50.0f);
		if (axistype == 0)
		{
			s = QVector3D(10.0f, 15.0f, 10.0f);
			geometry = PRIMITIVESHAPE("arrow");
		}
		else if (axistype == 1)
		{
			s = QVector3D(2.0f, 3.3f, 2.0f);
			geometry = PRIMITIVESHAPE("cylinder");
		}
		if (s_use != nullptr)
		{
			s = *s_use;
		}

		QMatrix4x4 xMatrix;
		xMatrix.rotate(-90.0f, 0.0f, 0.0f, 1.0f);
		xMatrix.scale(s);
		QMatrix4x4 yMatrix;
		yMatrix.scale(s);
		QMatrix4x4 zMatrix;
		zMatrix.rotate(90.0f, 1.0f, 0.0f, 0.0f);
		zMatrix.scale(s);
		m_xAxis->setPose(xMatrix);
		m_yAxis->setPose(yMatrix);
		m_zAxis->setPose(zMatrix);

		m_xAxis->setColor(QVector4D(1.0f, 0.0f, 0.0f, 1.0f));
		m_yAxis->setColor(QVector4D(0.0f, 1.0f, 0.0f, 1.0f));
		m_zAxis->setColor(QVector4D(0.0f, 0.0f, 1.0f, 1.0f));

		m_xAxis->setGeometry(geometry);
		m_yAxis->setGeometry(geometry);
		m_zAxis->setGeometry(geometry);


		QMatrix4x4 x_txtmatrix;
		TextMeshEntity* textEntity = new TextMeshEntity(this);
		textEntity->setColor(QVector4D(1.0f, 0.0f, 0.0f, 1.0f));
		textEntity->setText("X");
		x_txtmatrix.scale(3);
		x_txtmatrix.translate(6.0f, 0.0f, 0.0f);
		x_txtmatrix.rotate(90, 1, 0, 0);
		textEntity->setPose(x_txtmatrix);

		QMatrix4x4 y_txtmatrix;
		TextMeshEntity* textEntity2 = new TextMeshEntity(this);
		textEntity2->setColor(QVector4D(0.0f, 1.0f, 0.0f, 1.0f));
		textEntity2->setText("Y");
		y_txtmatrix.scale(3);
		y_txtmatrix.translate(-0.1f, 6.0f, 0.0f);
		y_txtmatrix.rotate(90, 1, 0, 0);
		textEntity2->setPose(y_txtmatrix);

		QMatrix4x4 z_txtmatrix;
		TextMeshEntity* textEntity3 = new TextMeshEntity(this);
		textEntity3->setColor(QVector4D(0.0f, 0.0f, 1.0f, 1.0f));
		textEntity3->setText("Z");
		z_txtmatrix.scale(3);
		z_txtmatrix.translate(-0.1f, 0.0f, 6.0f);
		z_txtmatrix.rotate(90, 1, 0, 0);
		textEntity3->setPose(z_txtmatrix);

		m_xText = textEntity;
		m_yText = textEntity2;
		m_zText = textEntity3;

		//m_xAxis->setPassDepthTest("pure", Qt3DRender::QDepthTest::Always);
		//m_yAxis->setPassDepthTest("pure", Qt3DRender::QDepthTest::Always);
		//m_zAxis->setPassDepthTest("pure", Qt3DRender::QDepthTest::Always);
		//textEntity->setPassDepthTest("pure", Qt3DRender::QDepthTest::Always);
		//textEntity2->setPassDepthTest("pure", Qt3DRender::QDepthTest::Always);
		//textEntity3->setPassDepthTest("pure", Qt3DRender::QDepthTest::Always);
	}
	
	AxisEntity::~AxisEntity()
	{
	}

	void AxisEntity::translate(QVector3D v)
	{
		QMatrix4x4 t;
		t.translate(v.x(), v.y());

		QMatrix4x4 xMatrix = m_xAxis->pose();
		xMatrix = t * xMatrix;
		m_xAxis->setPose(xMatrix);

		QMatrix4x4 yMatrix = m_yAxis->pose();
		yMatrix = t * yMatrix;
		m_yAxis->setPose(yMatrix);

		QMatrix4x4 zMatrix = m_zAxis->pose();
		zMatrix = t * zMatrix;
		m_zAxis->setPose(zMatrix);
	}

	void AxisEntity::setXAxisColor(const QVector4D& color)
	{
		m_xAxis->setColor(color);
		m_xText->setColor(color);
	}

	void AxisEntity::setYAxisColor(const QVector4D& color)
	{
		m_yAxis->setColor(color);
		m_yText->setColor(color);
	}
	
	void AxisEntity::setZAxisColor(const QVector4D& color)
	{
		m_zAxis->setColor(color);
		m_zText->setColor(color);
	}
}