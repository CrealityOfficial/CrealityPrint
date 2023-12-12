#include "slicepreviewscene.h"
#include "entity/purecolorentity.h"
#include "qtuser3d/camera/screencamera.h"
#include "entity/alonepointentity.h"
#include "data/kernelmacro.h"
#include "internal/render/slicepreviewnode.h"
#include "interface/reuseableinterface.h"
#include "interface/camerainterface.h"
#include <cmath>
#include "entity/nozzleentity.h"
using namespace qtuser_3d;
namespace creative_kernel
{
	SlicePreviewScene::SlicePreviewScene(Qt3DCore::QNode* parent)
		:QEntity(parent),
		m_layerHeight(0.1),
		m_lineWidth(0.4)
	{
		m_slicePreviewNode = new SlicePreviewNode(this);
		m_fdmIndicator = new NozzleEntity(this);

#if SHOW_ZSEAM_POINTS
		m_showZseam= false;
		m_showRetraction=false;

#if 0
		m_retractionPoints = new PureColorEntity(this);
		m_retractionPoints->setEffect(EFFECT("pointInGcode"));
		m_retractionPoints->setColor(QVector4D(1.0f, 0.0f, 1.0f, 1.0));

		m_zSeamsPoints = new PureColorEntity(this);
		m_zSeamsPoints->setEffect(EFFECT("pointInGcode"));
		m_zSeamsPoints->setColor(QVector4D(1.0f, 1.0f, 1.0f, 1.0));
#else
		m_retractionPoints = new AlonePointEntity(this);
		m_retractionPoints->setPointSize(2.0);
		m_retractionPoints->setShader("pointInGcode");
		m_retractionPoints->setColor(QVector4D(1.0f, 0.0f, 1.0f, 1.0));

		m_zSeamsPoints = new AlonePointEntity(this);
		m_zSeamsPoints->setPointSize(2.0);
		m_zSeamsPoints->setShader("pointInGcode");
		m_zSeamsPoints->setColor(QVector4D(1.0f, 1.0f, 1.0f, 1.0));
#endif // 0

#endif
	}

	SlicePreviewScene::~SlicePreviewScene()
	{
		clear();
	}

	void SlicePreviewScene::initialize()
	{
		m_slicePreviewNode->initialize();

		qtuser_3d::ScreenCamera* sc = visCamera();
		Qt3DRender::QCamera* camera = sc->camera();
		connect(camera, SIGNAL(projectionMatrixChanged(const QMatrix4x4 &)), this, SLOT(cameraProjectionMatrixChanged(const QMatrix4x4&)));
	}

	void SlicePreviewScene::clear()
	{
	}

	void SlicePreviewScene::setGCodeVisualType(gcode::GCodeVisualType type)
	{
		m_slicePreviewNode->setGCodeVisualType((int)type);
	}

	void SlicePreviewScene::setAnimation(int animation)
	{
		m_slicePreviewNode->setAnimation(animation);
#if SHOW_ZSEAM_POINTS
		m_retractionPoints->setParameter("animation", animation);
		m_zSeamsPoints->setParameter("animation", animation);
#endif
	}

	void SlicePreviewScene::visual()
	{
		m_slicePreviewNode->setEnabled(true);
		m_fdmIndicator->setEnabled(true);
	}

	void SlicePreviewScene::setOnlyLayer(int layer)
	{
		m_slicePreviewNode->setLayerShowScope(layer, layer);
#if SHOW_ZSEAM_POINTS
		m_retractionPoints->setParameter("layershow", QVector2D(layer, layer));
		m_zSeamsPoints->setParameter("layershow", QVector2D(layer, layer));
#endif
	}

	void SlicePreviewScene::unsetOnlyLayer()
	{
		m_slicePreviewNode->clearLayerShowScope();
#if SHOW_ZSEAM_POINTS
		m_retractionPoints->setParameter("layershow", QVector2D(-1.0f, 9999999.0f));
		m_zSeamsPoints->setParameter("layershow", QVector2D(-1.0f, 9999999.0f));
#endif
	}

	void SlicePreviewScene::setClipValue(const QVector4D& clipValue)
	{
		m_slicePreviewNode->setClipValue(clipValue);
#if SHOW_ZSEAM_POINTS
		m_retractionPoints->setParameter("clipValue", clipValue);
		m_zSeamsPoints->setParameter("clipValue", clipValue);
#endif
	}

	void SlicePreviewScene::setIndicatorPos(const QVector3D& position)
	{
		QMatrix4x4 tansMatrix;
		tansMatrix.translate(position);
		m_fdmIndicator->setPose(tansMatrix);
	}

	SlicePreviewNode* SlicePreviewScene::previewNode()
	{
		return m_slicePreviewNode;
	}

	void SlicePreviewScene::setGeometry(Qt3DRender::QGeometry* geometry, Qt3DRender::QGeometryRenderer::PrimitiveType type, int vCountPerPatch)
	{
		m_slicePreviewNode->setGeometry(geometry, Qt3DRender::QGeometryRenderer::Points);
	}

	void SlicePreviewScene::setRetractionGeometryRenderer(Qt3DRender::QGeometryRenderer* renderer)
	{
#if SHOW_ZSEAM_POINTS
		m_retractionPoints->replaceGeometryRenderer(renderer);
#endif
	}

	void SlicePreviewScene::setZSeamsGeometryRenderer(Qt3DRender::QGeometryRenderer* renderer)
	{
#if SHOW_ZSEAM_POINTS
		m_zSeamsPoints->replaceGeometryRenderer(renderer);
#endif
	}

	void SlicePreviewScene::setRetractionGeometry(Qt3DRender::QGeometry* geometry, Qt3DRender::QGeometryRenderer::PrimitiveType type, int vCountPerPatch)
	{
#if SHOW_ZSEAM_POINTS
		m_retractionPoints->setGeometry(geometry, type, true);
#endif
	}

	void SlicePreviewScene::setZSeamsGeometry(Qt3DRender::QGeometry* geometry, Qt3DRender::QGeometryRenderer::PrimitiveType type, int vCountPerPatch)
	{
#if SHOW_ZSEAM_POINTS
		m_zSeamsPoints->setGeometry(geometry, type, true);
#endif
	}

	void SlicePreviewScene::setRetractionVisible()
	{
#if SHOW_ZSEAM_POINTS
		m_retractionPoints->setEnabled(m_showRetraction);
#endif
	}

	void SlicePreviewScene::setZSeamsVisible()
	{
#if SHOW_ZSEAM_POINTS
		m_zSeamsPoints->setEnabled(m_showZseam);
#endif
	}

	void SlicePreviewScene::setZseamRetractDis()
	{
#if SHOW_ZSEAM_POINTS
		m_zSeamsPoints->setEnabled(false);
		m_retractionPoints->setEnabled(false);
#endif
	}

	void SlicePreviewScene::setPreviewNodeModelMatrix(const QMatrix4x4& matrix)
	{
		m_slicePreviewNode->setModelMatrix(matrix);

#if SHOW_ZSEAM_POINTS
		m_zSeamsPoints->setParameter("modelMatrix", matrix);
		m_retractionPoints->setParameter("modelMatrix", matrix);
#endif
	}

	void SlicePreviewScene::setIndicatorVisible(bool isShow)
	{
		m_fdmIndicator->setEnabled(isShow);
	}

	void SlicePreviewScene::showGCodeType(int type, bool isShow)
	{
		m_slicePreviewNode->setTypeShow(type, isShow);
		if (type==17)
		{
			m_showZseam = isShow;
			setZSeamsVisible();
		} 
		else if (type == 18)
		{
			m_showRetraction = isShow;
			setRetractionVisible();
		}
	}

	// 根据相机信息预估出一个合适的圆半径
	float SlicePreviewScene::caculateAdaptivePointSize()
	{
		if (m_lineWidth <= 0.0 || m_layerHeight <= 0.0)
		{
			return 2.0;
		}

		qtuser_3d::ScreenCamera* sc = visCamera();

		QVector3D origin = sc->orignCenter();
		QVector3D right = sc->horizontal();
		QVector3D p = origin + right.normalized() * 1.0;

		const QMatrix4x4 vp = sc->projectionMatrix() * sc->viewMatrix();

		QVector4D clipO = vp * QVector4D(origin, 1.0);
		QVector3D ndcO = QVector3D(clipO) / clipO.w();
		QVector2D uvO = QVector2D(ndcO);

		QVector4D clipP = vp * QVector4D(p, 1.0);
		QVector3D ndcP = QVector3D(clipP) / clipP.w();
		QVector2D uvP = QVector2D(ndcP);

		float scaleX = uvP.x() - uvO.x();
		float pixels = sc->size().width() * scaleX * 0.5;

		//不同观察角度大小不一样，垂直方向看到的大概是线宽的大小，水平方向大致是层高般大小
		{
			float lineWidth = m_lineWidth, layerHeight = m_layerHeight * 2.0;
			float maxSize = lineWidth * 1.25, minSize = layerHeight * 1.25;

			QVector3D viewDir = sc->camera()->viewVector().normalized();
			float k = QVector3D::dotProduct(viewDir, QVector3D(0.0, 0.0, 1.0));
			k = abs(k);
			k = minSize + (maxSize - minSize) * k;
			pixels *= k;
		}

		pixels = fmax(pixels, 2.0);
		pixels = ceil(pixels);

		return pixels;
	}

	void SlicePreviewScene::updatePointEntitySize()
	{
#if SHOW_ZSEAM_POINTS
		float pixels = caculateAdaptivePointSize();

		m_retractionPoints->setPointSize(pixels);
		m_zSeamsPoints->setPointSize(pixels);
#endif
	}

	void SlicePreviewScene::cameraProjectionMatrixChanged(const QMatrix4x4& mat)
	{
		updatePointEntitySize();
	}

	void SlicePreviewScene::setLayerHeight(float height)
	{
		m_layerHeight = height;

		m_slicePreviewNode->setLayerHeight(height);

		updatePointEntitySize();
	}

	void SlicePreviewScene::setLineWidth(float width)
	{
		m_lineWidth = width;

		//m_slicePreviewNode->setLineWidth(width);

		updatePointEntitySize();
	}
}