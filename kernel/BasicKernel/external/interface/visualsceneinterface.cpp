#include "visualsceneinterface.h"
#include "external/kernel/kernel.h"
#include "external/kernel/visualscene.h"
#include "qtuser3d/module/facepicker.h"
#include "external/kernel/kernelui.h"
#include "operation/moveoperatemode.h"

namespace creative_kernel
{
	Qt3DCore::QEntity* spaceEntity()
	{
		return getKernel()->visScene()->spaceEntity();
	}

	void spaceShow(Qt3DCore::QEntity* entity)
	{
		getKernel()->visScene()->show(entity, VisualScene::ModelSpace);
	}

	void spaceHide(Qt3DCore::QEntity* entity)
	{
		getKernel()->visScene()->hide(entity);
	}

	void requestCapture(Qt3DRender::QCamera* camera, QObject* receiver, qtuser_3d::RenderCaptor::ReceiverHandleReplyFunc func)
	{
		getKernel()->visScene()->requestCapture(camera, receiver, func);
	}

	bool isUsingDefaultScene()
	{
		return getKernel()->visScene()->isDefaultScene();
	}

	bool isUsingPreviewScene()
	{
		return getKernel()->visScene()->isPreviewScene();
	}

	void usePrepareScene()
	{
		getKernel()->visScene()->useScene(VisualScene::Default | VisualScene::ModelSpace);
	}

	void usePreviewScene()
	{
		getKernel()->visScene()->useScene(VisualScene::Default | VisualScene::Preview | VisualScene::ModelSpace);
	}

	void useCustomScene()
	{
		getKernel()->visScene()->useScene(VisualScene::Custom);
	}

	void setPreviewEnabled(bool enabled)
	{
		getKernel()->visScene()->setPreviewEnabled(enabled);
	}

	void clearRenderRequestors()
	{
		getKernel()->visScene()->clearRenderRequestors();
	}

	void requestRender(QObject* requestor)
	{
		getKernel()->visScene()->requestRender(requestor);
	}

	void endRequestRender(QObject* requestor)
	{
		getKernel()->visScene()->endRequestRender(requestor);
	}

	void setVisOperationMode(qtuser_3d::SceneOperateMode* operateMode)
	{
		static MoveOperateMode* defaultOperateMode = new MoveOperateMode;
		if (operateMode == NULL)
			getKernel()->visScene()->setOperateMode(defaultOperateMode);
		else
			getKernel()->visScene()->setOperateMode(operateMode);
	}

	qtuser_3d::SceneOperateMode* visOperationMode()
	{
		return getKernel()->visScene()->operateMode();
	}

	void visShowCustom(Qt3DCore::QEntity* entity)
	{
		getKernel()->visScene()->show(entity, VisualScene::Custom);
	}

	void visHideCustom(Qt3DCore::QEntity* entity)
	{
		getKernel()->visScene()->hide(entity);
	}

	void visShow(Qt3DCore::QEntity* entity)
	{
		getKernel()->visScene()->show(entity, VisualScene::Default);
	}

	void visHide(Qt3DCore::QEntity* entity)
	{
		getKernel()->visScene()->hide(entity);
	}

	void enableVisHandlers(bool enabled)
	{
		getKernel()->visScene()->enableHandler(enabled);
	}

	void setBoxEnabled(bool enabled)
	{
		getKernel()->visScene()->setBoxEnabled(enabled);
	}

	void forceHideBox(bool enabled)
	{
		getKernel()->visScene()->forceHideBox(enabled);
	}

	void delayCapture(int msec)
	{
		getKernel()->visScene()->delayCapture(msec);
	}

	void requestVisUpdate(bool capture)
	{
		VisualScene* visScene = getKernel()->visScene();
		visScene->updateRender(capture);
	}

	void requestVisPickUpdate(bool sync)
	{
		VisualScene* visScene = getKernel()->visScene();
		visScene->updatePick(sync);
	}

	qtuser_3d::FacePicker* visPickSource()
	{
		return getKernel()->visScene()->facePicker();
	}

	bool visPick(const QPoint& point, int* faceID)
	{
		return visPickSource()->pick(point, faceID);
	}

	bool shouldMultipleSelect()
	{
		return getKernel()->visScene()->shouldMultipleSelect();
	}

	void showRectangleSelector(const QRect& rect)
	{
		getKernel()->visScene()->showRectangleSelector(rect);
	}
	
	void dismissRectangleSelector()
	{
		getKernel()->visScene()->dismissRectangleSelector();
	}

	bool didSelectAnyEntity(const QPoint& p)
	{
		return getKernel()->visScene()->didSelectAnyEntity(p);
	}

	void showPrimeTower(float x, float y, float radius)
	{
	}

	void hidePrimeTower()
	{
	}

	QSize surfaceSize()
	{
		return getKernel()->visScene()->surfaceSize();
	}

	void updateSceneSelectionsGlobalBox(const qtuser_3d::Box3D& box)
	{
		getKernel()->visScene()->updateSelectionsGlobalBox(box);
	}

	void setModelVisualMode(ModelVisualMode mode)
	{
		getKernel()->visScene()->setModelVisualMode(mode);
	}

	ModelVisualMode modelVisualMode()
	{
		return getKernel()->visScene()->modelVisualMode();
	}
}
