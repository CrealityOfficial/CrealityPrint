#ifndef _CAPTUREHELPER_1596613436897_H
#define _CAPTUREHELPER_1596613436897_H
#include "shader_entity_export.h"
#include <Qt3DRender/QFrameGraphNode>
#include <Qt3DRender/QGeometry>
#include <Qt3DRender/QTexture>
#include <Qt3DRender/QEffect>

#include "qtuser3d/math/box3d.h"
#include "utils/captureSetupData.h"
#include "utils/capturetask.h"

#include "entity/pureentity.h"

namespace qtuser_3d
{
	class CaptureEntity;
	class ColorPicker;
	class TextureRenderTarget;
	class XEntity;
	class SHADER_ENTITY_API CaptureHelper : public QObject
	{
		Q_OBJECT
	public:
		CaptureHelper(QObject* parent = nullptr);
		virtual ~CaptureHelper();

		void resize(qtuser_3d::Box3D& box, int resolution_x, int resolution_y);
		void attachToMainGraph(Qt3DRender::QFrameGraphNode* frameGraph, Qt3DCore::QNode* sceneGraph);
		void detachFromMainGraph();
		void clear();

		void setFormat(Qt3DRender::QAbstractTexture::TextureFormat format);
		void capture(Qt3DRender::QGeometry* geometry, QString index, captureCallbackFunc func);

		void captureModel(const CaptureSetupData& captureSetInfo, captureCallbackFunc func, bool bProject = false);

		void capturePreview(captureCallbackFunc func, QVector3D& viewCenter, QVector3D& upVector, QVector3D& eyePosition, QMatrix4x4& projectionMatrix, QString name = "-1");
		void onPreviewCaptureFinish();

		void captureModelComplete();

		qtuser_3d::ColorPicker* colorPicker();

	protected:
		void captureComplete(QImage& image);

		//void createCaptureEffect();

	protected:
		qtuser_3d::ColorPicker* m_colorPicker;
		qtuser_3d::PureEntity* m_basicEntity;
		qtuser_3d::TextureRenderTarget* m_renderTarget;

		Qt3DCore::QNode* m_sceneGraph;

		//qtuser_3d::BasicEntity* m_captureEntity;
		//Qt3DRender::QEffect* m_captureEffect;

		qtuser_3d::CaptureEntity* m_captureEntity;

		QString m_index;
		captureCallbackFunc m_func;

	};

}

#endif // _CAPTUREHELPER_1596613436897_H