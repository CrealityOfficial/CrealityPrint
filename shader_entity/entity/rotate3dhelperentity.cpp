#include "entity/rotate3dhelperentity.h"

#include "entity/manipulateentity.h"
#include "qtuser3d/module/manipulatepickable.h"

#include "qtuser3d/module/facepicker.h"
#include "qtuser3d/camera/screencamera.h"
#include "qtuser3d/math/angles.h"

#include "rotatehelperentity_T.h"

#include <Qt3DExtras/QTorusMesh>
#include <QtCore/qmath.h>


namespace qtuser_3d
{
	Rotate3DHelperEntity::Rotate3DHelperEntity(Qt3DCore::QNode* parent)
		: XEntity(parent)
		, m_pXRotHelper(nullptr)
		, m_pYRotHelper(nullptr)
		, m_pZRotHelper(nullptr)
		, m_pRotateCallback(nullptr)
	{
		m_pXRotHelper = new RotateHelperEntity_T(this);
		m_pXRotHelper->setColor(QVector4D(1.0f, 0.1098f, 0.1098f, 1.0f));
		m_pXRotHelper->setRotateAxis(QVector3D(1.0, 0.0, 0.0));
		m_pXRotHelper->setRotateCallback(this);

		m_pYRotHelper = new RotateHelperEntity_T(this);
		m_pYRotHelper->setColor(QVector4D(0.247f, 0.933f, 0.1098f, 1.0f));
		m_pYRotHelper->setRotateAxis(QVector3D(0.0, 1.0, 0.0));
		//m_pYRotHelper->setRotateInitAngle(90);
		m_pYRotHelper->setRotateCallback(this);

		m_pZRotHelper = new RotateHelperEntity_T(this);
		m_pZRotHelper->setColor(QVector4D(0.4117f, 0.243f, 1.0f, 1.0f));
		m_pZRotHelper->setRotateAxis(QVector3D(0.0, 0.0, 1.0), 90.0);
		//m_pZRotHelper->setRotateInitAngle(90);
		m_pZRotHelper->setRotateCallback(this);
	}

	Rotate3DHelperEntity::~Rotate3DHelperEntity()
	{
	}

	QList<Pickable*> Rotate3DHelperEntity::getPickables()
	{
		QList<Pickable*> pickables;
		if (m_pXRotHelper)
			pickables << m_pXRotHelper->getPickable();

		if (m_pYRotHelper)
			pickables << m_pYRotHelper->getPickable();

		if (m_pZRotHelper)
			pickables << m_pZRotHelper->getPickable();
		
		return pickables;
	}

	Pickable* Rotate3DHelperEntity::xPickable()
	{
		if (m_pXRotHelper)
			return m_pXRotHelper->getPickable();
		return nullptr;
	}
	
	Pickable* Rotate3DHelperEntity::yPickable()
	{
		if (m_pYRotHelper)
			return m_pYRotHelper->getPickable();
		return nullptr;
	}

	Pickable* Rotate3DHelperEntity::zPickable()
	{
		if (m_pZRotHelper)
			return m_pZRotHelper->getPickable();
		return nullptr;
	}

	QList<LeftMouseEventHandler*> Rotate3DHelperEntity::getLeftMouseEventHandlers()
	{
		QList<LeftMouseEventHandler*> handlers;

		if (m_pXRotHelper)
			handlers << m_pXRotHelper;

		if (m_pYRotHelper)
			handlers << m_pYRotHelper;

		if (m_pZRotHelper)
			handlers << m_pZRotHelper;

		return handlers;
	}

	void Rotate3DHelperEntity::setPickSource(FacePicker* pickSource)
	{
		if (m_pXRotHelper)
			m_pXRotHelper->setPickSource(pickSource);

		if (m_pYRotHelper)
			m_pYRotHelper->setPickSource(pickSource);

		if (m_pZRotHelper)
			m_pZRotHelper->setPickSource(pickSource);
	}

	void Rotate3DHelperEntity::setScreenCamera(ScreenCamera* camera)
	{
		if (m_pXRotHelper)
			m_pXRotHelper->setScreenCamera(camera);

		if (m_pYRotHelper)
			m_pYRotHelper->setScreenCamera(camera);

		if (m_pZRotHelper)
			m_pZRotHelper->setScreenCamera(camera);
	}

	void Rotate3DHelperEntity::setRotateCallback(RotateCallback* callback)
	{
		m_pRotateCallback = callback;
	}

	void Rotate3DHelperEntity::setXVisibility(bool visibility)
	{
		if (m_pXRotHelper)
			m_pXRotHelper->setVisibility(visibility);
	}

	void Rotate3DHelperEntity::setYVisibility(bool visibility)
	{
		if (m_pYRotHelper)
			m_pYRotHelper->setVisibility(visibility);
	}

	void Rotate3DHelperEntity::setZVisibility(bool visibility)
	{
		if (m_pZRotHelper)
			m_pZRotHelper->setVisibility(visibility);
	}

	void Rotate3DHelperEntity::setScale(float scaleRate)
	{
		if (m_pXRotHelper)
			m_pXRotHelper->setScale(scaleRate);

		if (m_pYRotHelper)
			m_pYRotHelper->setScale(scaleRate);
		
		if (m_pZRotHelper)
			m_pZRotHelper->setScale(scaleRate);
	}

	void Rotate3DHelperEntity::setInitScale(float initScale)
	{
		if (m_pXRotHelper)
			m_pXRotHelper->setInitScale(initScale);

		if (m_pYRotHelper)
			m_pYRotHelper->setInitScale(initScale);

		if (m_pZRotHelper)
			m_pZRotHelper->setInitScale(initScale);
	}

	QVector3D Rotate3DHelperEntity::getCurrentRotateAxis()
	{
		if (m_pXRotHelper->isRotating())
		{
			return m_pXRotHelper->getRotateAxis();
		}
		else if (m_pYRotHelper->isRotating())
		{
			return m_pYRotHelper->getRotateAxis();
		}
		else if (m_pZRotHelper->isRotating())
		{
			return m_pZRotHelper->getRotateAxis();
		}

		return QVector3D();
	}

	double Rotate3DHelperEntity::getCurrentRotAngle()
	{
		if (m_pXRotHelper->isRotating())
		{
			return m_pXRotHelper->getLastestRotAngle();
		}
		else if (m_pYRotHelper->isRotating())
		{
			return m_pYRotHelper->getLastestRotAngle();
		}
		else if (m_pZRotHelper->isRotating())
		{
			return m_pZRotHelper->getLastestRotAngle();
		}

		return 0.0;
	}

	//Ä£ÐÍ°üÎ§ºÐ
	void Rotate3DHelperEntity::onBoxChanged(Box3D box)
	{
		if (m_pXRotHelper)
			m_pXRotHelper->onBoxChanged(box);

		if (m_pYRotHelper)
			m_pYRotHelper->onBoxChanged(box);

		if (m_pZRotHelper)
			m_pZRotHelper->onBoxChanged(box);
	}
	bool Rotate3DHelperEntity::shouldStartRotate()
	{
		if (m_pRotateCallback)
		{
			return m_pRotateCallback->shouldStartRotate();
		}
		return true;
	}
	void Rotate3DHelperEntity::onStartRotate()
	{
		if (m_pRotateCallback)
			m_pRotateCallback->onStartRotate();

		if (m_pXRotHelper)
		{
			m_pXRotHelper->setHandlerVisibility(m_pXRotHelper->isRotating());
			m_pXRotHelper->setDialVisibility(m_pXRotHelper->isRotating());
		}

		if (m_pYRotHelper)
		{
			m_pYRotHelper->setHandlerVisibility(m_pYRotHelper->isRotating());
			m_pYRotHelper->setDialVisibility(m_pYRotHelper->isRotating());
		}

		if (m_pZRotHelper)
		{
			m_pZRotHelper->setHandlerVisibility(m_pZRotHelper->isRotating());
			m_pZRotHelper->setDialVisibility(m_pZRotHelper->isRotating());
		}
	}

	void Rotate3DHelperEntity::onRotate(QQuaternion q)
	{
		if (m_pRotateCallback)
			m_pRotateCallback->onRotate(q);
	}

	void Rotate3DHelperEntity::onEndRotate(QQuaternion q)
	{
		if (m_pRotateCallback)
			m_pRotateCallback->onEndRotate(q);

		if (m_pXRotHelper)
		{
			m_pXRotHelper->setHandlerVisibility(true);
			m_pXRotHelper->setDialVisibility(false);
		}

		if (m_pYRotHelper)
		{
			m_pYRotHelper->setHandlerVisibility(true);
			m_pYRotHelper->setDialVisibility(false);
		}

		if (m_pZRotHelper)
		{
			m_pZRotHelper->setHandlerVisibility(true);
			m_pZRotHelper->setDialVisibility(false);
		}
	}

	void Rotate3DHelperEntity::setRotateAngle(QVector3D axis, float angle)
	{
		if (m_pRotateCallback)
			m_pRotateCallback->setRotateAngle(axis, angle);
	}
}
