#include "visualsceneinterface.h"
#include "external/kernel/kernel.h"
#include "external/kernel/visualscene.h"
#include "qtuser3d/module/facepicker.h"
#include "external/kernel/kernelui.h"
#include "operation/moveoperatemode.h"

namespace creative_kernel
{
	bool isUsingDefaultScene()
	{
		return getKernel()->visScene()->isDefaultScene();
	}

	void useCustomScene()
	{
		getKernel()->visScene()->useCustomScene();
	}

	void useDefaultScene()
	{
		getKernel()->visScene()->useDefaultScene();
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
		getKernel()->visScene()->showCustom(entity);
	}

	void visHideCustom(Qt3DCore::QEntity* entity)
	{
		getKernel()->visScene()->hideCustom(entity);
	}

	void visShow(Qt3DCore::QEntity* entity)
	{
		getKernel()->visScene()->show(entity);
	}

	void visHide(Qt3DCore::QEntity* entity)
	{
		getKernel()->visScene()->hide(entity);
	}

	void enableVisHandlers(bool enabled)
	{
		getKernel()->visScene()->enableHandler(enabled);
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
		getKernel()->visScene()->showPrimeTower(x, y, radius);
	}

	void hidePrimeTower()
	{
		getKernel()->visScene()->hidePrimeTower();
	}

	QSize surfaceSize()
	{
		return getKernel()->visScene()->surfaceSize();
	}
}
