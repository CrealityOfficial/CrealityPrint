#include "capturecontainer.h"
#include <QThread>
#include "utils/captureSetupData.h"
#include "utils/capturetask.h"
#include "utils/capturehelper.h"


namespace qtuser_3d
{
	CaptureContainer::CaptureContainer(QObject* parent)
		:QObject(parent)
		, m_previewHelper(nullptr)
	{
		
	}

	CaptureContainer::~CaptureContainer()
	{
	}

	void CaptureContainer::setRoot(Qt3DRender::QFrameGraphNode* frameGraph, Qt3DCore::QNode* sceneGraph)
	{

		if (m_previewHelper == nullptr)
		{
			m_previewHelper = new qtuser_3d::CaptureHelper(this);
		}
		m_previewHelper->attachToMainGraph(frameGraph, sceneGraph);
	}

	void CaptureContainer::start(qtuser_3d::Box3D& box, int resolution_x, int resolution_y, Qt3DRender::QFrameGraphNode* frameGraph, Qt3DCore::QNode* sceneGraph)
	{
		if (m_previewHelper == nullptr)
		{
			m_previewHelper = new qtuser_3d::CaptureHelper(this);
			m_previewHelper->attachToMainGraph(frameGraph, sceneGraph);
		}
		
		m_previewHelper->resize(box, resolution_x, resolution_y);
	}

	void CaptureContainer::finish()
	{
	}

	void CaptureContainer::executePreivew(CaptureTask* task, QVector3D& viewCenter, QVector3D& upVector, QVector3D& eyePosition, QMatrix4x4& projectionMatrix)
	{
		if (!m_previewHelper)
			return;

		m_previewHelper->capturePreview(task->m_func, viewCenter, upVector, eyePosition, projectionMatrix, QString::number(task->m_state));
	}

	void CaptureContainer::onPreviewCaptureFinish()
	{
		if (m_previewHelper)
		{
			m_previewHelper->onPreviewCaptureFinish();
		}
	}

	void CaptureContainer::captureScene(CaptureTask* task, QString scene_name, qtuser_3d::Box3D box)
	{
		QMatrix4x4 pMatrix;
		QVector3D dir(0.0f, 1.0f, -1.0f);
		QVector3D right(1.0f, 0.0f, 0.0f);
		QVector3D newUp = QVector3D::crossProduct(right, dir);
		newUp.normalize();
		// newUp = QVector3D(0.0f, 0.0f, 1.0f);

		QVector3D bsize = box.size();
		QVector3D center = box.center();
		float angle = 30.0f;
		float radius = bsize.length() / 2.0f;
		float newDistance = radius / sinf(angle * 3.14159265 / 180.0f / 2.0f);

		QVector3D newPosition = center - dir.normalized() * newDistance;
		float nearPlane = newDistance - radius;
		float farPlane = newDistance + radius;

		pMatrix.setToIdentity();
		pMatrix.perspective(angle, 1.0f, nearPlane, farPlane);

		m_previewHelper->capturePreview(task->m_func, center, newUp, newPosition, pMatrix, scene_name);
	}

	qtuser_3d::ColorPicker* CaptureContainer::colorPicker()
	{
		if (m_previewHelper)
			return m_previewHelper->colorPicker();

		return nullptr;
	}

	void CaptureContainer::captureModel(qtuser_3d::CaptureTask* task, QString model_name, Qt3DRender::QGeometry* geometry, QMatrix4x4 entityPos, qtuser_3d::Box3D box)
	{
		CaptureSetupData captureSetInfo;

		QVector3D dir(0.0f, 1.0f, -1.0f);
		QVector3D right(1.0f, 0.0f, 0.0f);
		captureSetInfo.upVector = QVector3D::crossProduct(right, dir);
		captureSetInfo.upVector.normalize();

		QVector3D bsize = box.size();
		captureSetInfo.viewCenter = box.center();
		float angle = 45.0f;
		float radius = bsize.length() / 2.0f;
		float newDistance = 1.3f * radius / sinf(angle * 3.14159265 / 180.0f / 2.0f) ;

		captureSetInfo.eyePosition = captureSetInfo.viewCenter - dir.normalized() * newDistance;
		float nearPlane = newDistance - radius;
		float farPlane = newDistance + radius;

		captureSetInfo.projectionMatrix.setToIdentity();
		captureSetInfo.projectionMatrix.perspective(angle, 1.0f, nearPlane, farPlane);

		captureSetInfo.geometry = geometry;
		captureSetInfo.entityPos = entityPos;
		captureSetInfo.captureImageName = model_name;

		m_previewHelper->captureModel(captureSetInfo, task->m_func);
	}

	void CaptureContainer::captureProjectModels(qtuser_3d::CaptureTask* task, QString model_name, qtuser_3d::Box3D box)
	{
		CaptureSetupData captureSetInfo;

		QVector3D dir(0.0f, 1.0f, -1.0f);
		QVector3D right(1.0f, 0.0f, 0.0f);
		captureSetInfo.upVector = QVector3D::crossProduct(right, dir);
		captureSetInfo.upVector.normalize();

		QVector3D bsize = box.size();
		captureSetInfo.viewCenter = box.center();
		float angle = 45.0f;
		float radius = bsize.length() / 2.0f;
		float newDistance = 1.3f * radius / sinf(angle * 3.14159265 / 180.0f / 2.0f);

		captureSetInfo.eyePosition = captureSetInfo.viewCenter - dir.normalized() * newDistance;
		float nearPlane = newDistance - radius;
		float farPlane = newDistance + radius;

		captureSetInfo.projectionMatrix.setToIdentity();
		captureSetInfo.projectionMatrix.perspective(angle, 1.0f, nearPlane, farPlane);

		captureSetInfo.captureImageName = model_name;

		if (m_previewHelper)
		{
			m_previewHelper->captureModel(captureSetInfo, task->m_func, true);
		}
		
	}

	void CaptureContainer::captureModelComplete()
	{
		if (m_previewHelper)
		{
			m_previewHelper->captureModelComplete();
		}
		
	}

}