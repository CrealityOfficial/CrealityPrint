#include "visualsceneinterface.h"
#include "external/kernel/kernel.h"
#include "external/kernel/visualscene.h"
#include "qtuser3d/module/facepicker.h"

namespace creative_kernel
{
	void setVisOperationMode(qtuser_3d::SceneOperateMode* operateMode)
	{
		getKernel()->visScene()->setOperateMode(operateMode);
	}

	qtuser_3d::SceneOperateMode* visOperationMode()
	{
		return getKernel()->visScene()->operateMode();
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