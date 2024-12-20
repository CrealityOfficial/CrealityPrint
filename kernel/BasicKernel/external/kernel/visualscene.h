#ifndef CREATIVE_KERNEL_VISUALSCENE_1592188654160_H
#define CREATIVE_KERNEL_VISUALSCENE_1592188654160_H
#include "data/header.h"
#include "data/kernelenum.h"
#include "data/interface.h"
#include "qtuser3d/framegraph/rendergraph.h"
#include "qtuser3d/framegraph/surface.h"
#include "qtuser3d/framegraph/rendercaptor.h"
#include <QtCore/QTimer>

#include "data/kernelenum.h"

namespace qtuser_3d
{
	class Surface;
	class FacePicker;
	class ColorPicker;
	class TextureRenderTarget;
	class RectLineEntity;
	class XEntity;
	class BoxEntity;
}

namespace creative_kernel
{
	class VisSceneHandler;
	class WipeTowerHandler;
	class ModelGroupEntity;
	class ModelNEntity;
	class BASIC_KERNEL_API VisualScene : public qtuser_3d::RenderGraph
		, public KernelPhase
	{
		Q_OBJECT
	signals:
		void sceneChanged();
		void pictureCaptureCompleted();
		void modelVisualModeChanged();

	public:
		enum Scene 
		{
			Default = 0x01,			//盘、指示器等
			ModelSpace = 0x02,		//模型
			Preview = 0x04,			//G-code预览
			Custom = 0x08			//自定义场景，如涂抹场景
		};

		VisualScene(Qt3DCore::QNode* parent = nullptr);
		virtual ~VisualScene();
		
		void requestCapture(Qt3DRender::QCamera* camera, QObject* receiver, qtuser_3d::RenderCaptor::ReceiverHandleReplyFunc func);

		void setPreviewScene(Qt3DCore::QEntity* entity);

		Qt3DCore::QEntity* spaceEntity();

		ModelGroupEntity* createModelGroupEntity();
		void destroyModelGroupEntity(ModelGroupEntity* entity);
		ModelNEntity* createModelNEntity();
		void destroyModelNEntity(ModelNEntity* entity);

		bool isDefaultScene();
		bool isPreviewScene();

		void useScene(int scenes);

		void show(Qt3DCore::QEntity* entity, Scene scene);
		void hide(Qt3DCore::QEntity* entity);

		void enableHandler(bool enable);

		qtuser_3d::FacePicker* facePicker();
		qtuser_3d::TextureRenderTarget* textureRenderTarget();

		void initialize();

		bool shouldMultipleSelect();
		bool didSelectAnyEntity(const QPoint& p);
		void showRectangleSelector(const QRect& rect);
		void dismissRectangleSelector();

		void delayCapture(int msec);
		
		Qt3DRender::QFrameGraphNode* getCameraViewportFrameGraphNode();

		QSize surfaceSize();

		void setSceneClearColor(const QColor& color);

		void updatePick(bool sync);

		void updateSelectionsGlobalBox(const qtuser_3d::Box3D& box);

		Qt3DRender::QCamera* getCaptureCamera();
		qtuser_3d::ColorPicker* getCaptor();

		void setBoxEnabled(bool enabled);
		void forceHideBox(bool enabled);

		void setPreviewEnabled(bool enabled);

		void clearRenderRequestors();
		void requestRender(QObject* requestor);

		/* model */
		void setModelVisualMode(ModelVisualMode mode);
		ModelVisualMode modelVisualMode();

	public slots:
		void endRequestRender(QObject* requestor);

		void updateRender(bool updatePick = false);

	public:
		Qt3DCore::QEntity* sceneGraph() override;

		void begineRender() override;
		void endRender() override;
		void updateRenderSize(const QSize& size) override;

		void onStartPhase() override;
		void onStopPhase() override;
#ifdef ENABLE_DEBUG_OVERLAY
		bool showDebugOverlay() override;
		void setShowDebugOverlay(bool showDebugOverlay) override;
#endif

		qtuser_3d::XEntity* heightReferEntity();
	private:
		qtuser_3d::Surface* m_surface;
		// qtuser_3d::Surface* m_captureSurface;
		Qt3DRender::QFrameGraphNode* m_colorPickerParent { NULL };
		bool m_captureFinished { true };

		qtuser_3d::ColorPicker* m_colorPicker;
		qtuser_3d::ColorPicker* m_captor;
		qtuser_3d::TextureRenderTarget* m_textureRenderTarget;
		qtuser_3d::TextureRenderTarget* m_captureRenderTarget;
		qtuser_3d::RectLineEntity* m_rectLine;
		qtuser_3d::XEntity* m_heightReferEntity;

		int m_captorCount { 0 };

		Qt3DCore::QEntity* m_rootEntity;
		Qt3DCore::QEntity* m_customRootEntity;
		Qt3DCore::QEntity* m_previewEntity{NULL};

		qtuser_3d::BoxEntity* m_boxEntity;
		bool m_forceHideBox { false };
		bool m_boxEnabled { false };

		VisSceneHandler* m_handler;
		WipeTowerHandler* m_towerHandler;

		bool m_handlerEnabled;

		QTimer* m_updateTimer;

		Qt3DCore::QEntity* m_space_root;

		QList<QObject*> m_renderRequestors;
	
		QList<ModelNEntity*> m_modelNEntitys;
		ModelVisualMode m_modelVisualMode { ModelVisualMode::mvm_face };

		QMap<QObject*, qtuser_3d::RenderCaptor*> m_requestCaptureMap;
		
	};
}
#endif // CREATIVE_KERNEL_VISUALSCENE_1592188654160_H
