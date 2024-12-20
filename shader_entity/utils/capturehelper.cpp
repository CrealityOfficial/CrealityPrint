#include "capturehelper.h"
#include "qtuser3d/framegraph/colorpicker.h"
#include "qtuser3d/framegraph/texturerendertarget.h"
#include "entity/capturexentity.h"
#include <Qt3DExtras/QPlaneGeometry>

namespace qtuser_3d
{
	CaptureHelper::CaptureHelper(QObject* parent)
		:QObject(parent)
		, m_index("0")
	{
		m_renderTarget = new qtuser_3d::TextureRenderTarget();

		m_colorPicker = new qtuser_3d::ColorPicker();

		m_colorPicker->useSelfCameraSelector(true);
		m_colorPicker->setEnabled(true);
		m_colorPicker->setClearColor(QColor(0, 0, 0, 255));
		m_colorPicker->setTextureRenderTarget(m_renderTarget);

		m_colorPicker->setRequestCallback(std::bind(&CaptureHelper::captureComplete, this, std::placeholders::_1));
		m_basicEntity = new qtuser_3d::PureEntity();
		m_basicEntity->setColor(QVector4D(1.0f, 1.0f, 1.0f, 1.0f));

		m_captureEntity = new qtuser_3d::CaptureEntity();
	}

	CaptureHelper::~CaptureHelper()
	{
	}

	void CaptureHelper::attachToMainGraph(Qt3DRender::QFrameGraphNode* frameGraph, Qt3DCore::QNode* sceneGraph)
	{
		m_renderTarget->setParent(sceneGraph);
		m_colorPicker->setParent(frameGraph);
		m_basicEntity->setParent(sceneGraph);
		m_captureEntity->setParent(sceneGraph);
	}

	void CaptureHelper::detachFromMainGraph()
	{
		clear();
	}

	void CaptureHelper::clear()
	{
		m_renderTarget->setParent((Qt3DCore::QNode*)nullptr);
		m_colorPicker->setParent((Qt3DCore::QNode*)nullptr);
		m_basicEntity->setParent((Qt3DCore::QNode*)nullptr);
		m_captureEntity->setParent((Qt3DCore::QNode*)nullptr);
	}

	void CaptureHelper::resize(qtuser_3d::Box3D& box, int resolution_x, int resolution_y)
	{
		QVector3D center = box.center();

		QVector3D boxSize = box.size();
		QSize textureSize(boxSize.x(), boxSize.y());

		///2020-08-15 liuhong modify for resolution_x
		//float pixelsize = 0.075f;
		m_renderTarget->resize(QSize(resolution_x, resolution_y));

		Qt3DRender::QCamera* pickerCamera = m_colorPicker->camera();
		pickerCamera->setViewCenter(QVector3D(center.x(), center.y(), 0.0f));
		pickerCamera->setPosition(QVector3D(center.x(), center.y(), box.max.z()));
		pickerCamera->setUpVector(QVector3D(0.0f, 1.0f, 0.0f));

		pickerCamera->setFarPlane(1000.0f);
		pickerCamera->setNearPlane(1.0f);
		pickerCamera->setProjectionType(Qt3DRender::QCameraLens::OrthographicProjection);
		pickerCamera->setBottom(-boxSize.y() / 2.0f);
		pickerCamera->setTop(boxSize.y() / 2.0f);
		pickerCamera->setLeft(-boxSize.x() / 2.0f);
		pickerCamera->setRight(boxSize.x() / 2.0f);
	}

	void CaptureHelper::setFormat(Qt3DRender::QAbstractTexture::TextureFormat format)
	{
		m_renderTarget->colorTexture()->setFormat(format);
	}

	void CaptureHelper::capture(Qt3DRender::QGeometry* geometry, QString index, captureCallbackFunc func)
	{
		qDebug() << "CaptureHelper::capture " << index;
		m_index = index;
		m_func = func;
		m_basicEntity->setGeometry(geometry);

		QString filter = "capture_" + index;     // QString("index%1").arg(index);
		m_colorPicker->setFilterKey(filter, 0);
		m_basicEntity->addPassFilter(0, filter);

		m_colorPicker->requestCapture();
	}

	void CaptureHelper::captureModel(const CaptureSetupData& captureSetInfo, captureCallbackFunc func, bool bProject)
	{
		if (!bProject)
		{
			if (!captureSetInfo.geometry)
				return;
		}

		qDebug() << "CaptureHelper::captureModel " << captureSetInfo.captureImageName;
		m_index = captureSetInfo.captureImageName;
		Qt3DRender::QCamera* pickerCamera = m_colorPicker->camera();
		pickerCamera->setProjectionType(Qt3DRender::QCameraLens::PerspectiveProjection);
		pickerCamera->setProjectionMatrix(captureSetInfo.projectionMatrix);
		pickerCamera->setUpVector(captureSetInfo.upVector);
		pickerCamera->setViewCenter(captureSetInfo.viewCenter);
		pickerCamera->setPosition(captureSetInfo.eyePosition);

		m_func = func;

		if (!bProject)
		{
			m_captureEntity->setGeometry(captureSetInfo.geometry);
			m_captureEntity->setPose(captureSetInfo.entityPos);
		}
		else
		{
			if (m_captureEntity)
				m_captureEntity->setGeometry(nullptr, Qt3DRender::QGeometryRenderer::Triangles, false);
		}

		m_colorPicker->setAllFilterKey(QStringLiteral("modelcapture"), 0);
		
		m_colorPicker->requestCapture();
	}

	void CaptureHelper::captureModelComplete()
	{
		if (m_captureEntity)
		{
			m_captureEntity->onCaptureComplete();
		}
	}

	qtuser_3d::ColorPicker* CaptureHelper::colorPicker()
	{
		return m_colorPicker;
	}

	void CaptureHelper::capturePreview(captureCallbackFunc func, QVector3D& viewCenter, QVector3D& upVector, QVector3D& eyePosition, QMatrix4x4& projectionMatrix, QString name)
	{
		qDebug() << "CaptureHelper::capturePreview ";
		Qt3DRender::QCamera* pickerCamera = m_colorPicker->camera();
		pickerCamera->setProjectionType(Qt3DRender::QCameraLens::PerspectiveProjection);
		pickerCamera->setProjectionMatrix(projectionMatrix);
		pickerCamera->setUpVector(upVector);
		pickerCamera->setViewCenter(viewCenter);
		pickerCamera->setPosition(eyePosition);

		m_index = name;
		m_func = func;
		m_colorPicker->requestCapture();

		m_colorPicker->setAllFilterKey("rt", 0);

	}

	void CaptureHelper::onPreviewCaptureFinish()
	{
		m_renderTarget->resize(QSize(90, 90));

		if (m_colorPicker)
		{
			m_colorPicker->setAllFilterKey("_pre_capture_finish_", 0);
		}
	}

	void CaptureHelper::captureComplete(QImage& image)
	{
		// under multi thread situation(import multi models at the same time), this could possibly cause exception, m_func may be invalid in "CaptureHelper::captureComplete"
		// modified by wys, 2023-4-7
		// 
		if (m_func)
			m_func(m_index, image);

	}

}