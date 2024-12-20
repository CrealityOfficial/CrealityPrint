#ifndef _CAPTURECONTAINER_1596608352857_H
#define _CAPTURECONTAINER_1596608352857_H
#include "shader_entity_export.h"
#include <QtCore/QObject>
#include <Qt3DRender/QFrameGraphNode>
#include <Qt3DRender/QGeometry>
#include "qtuser3d/math/box3d.h"
#include "qtuser3d/trimesh2/q3drender.h"

namespace qtuser_3d
{

	class CaptureTask;
	class CaptureHelper;
	class ColorPicker;
	class SHADER_ENTITY_API CaptureContainer : public QObject
	{
		Q_OBJECT
	public:
		CaptureContainer(QObject* parent = nullptr);
		virtual ~CaptureContainer();

		void setRoot(Qt3DRender::QFrameGraphNode* frameGrpha, Qt3DCore::QNode* sceneGraph);

		void start(qtuser_3d::Box3D& box, int resolution_x, int resolution_y, Qt3DRender::QFrameGraphNode* frameGraph, Qt3DCore::QNode* sceneGraph);
		void finish();
		void executePreivew(qtuser_3d::CaptureTask* task, QVector3D& viewCenter, QVector3D& upVector, QVector3D& eyePosition, QMatrix4x4& projectionMatrix);
		void onPreviewCaptureFinish();

		void captureModel(qtuser_3d::CaptureTask* task, QString model_name, Qt3DRender::QGeometry* geometry, QMatrix4x4 entityPos, qtuser_3d::Box3D box);
		void captureProjectModels(qtuser_3d::CaptureTask* task, QString model_name, qtuser_3d::Box3D box);
		void captureScene(qtuser_3d::CaptureTask* task, QString scene_name, qtuser_3d::Box3D box);

		qtuser_3d::ColorPicker* colorPicker();

	public slots:
		void captureModelComplete();
	protected:

		qtuser_3d::CaptureHelper* m_previewHelper;
	};

}

#endif // _CAPTURECONTAINER_1596608352857_H