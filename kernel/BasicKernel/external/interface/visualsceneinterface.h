#ifndef CREATIVE_KERNEL_VISUALSCENEINTERFACE_1592728018928_H
#define CREATIVE_KERNEL_VISUALSCENEINTERFACE_1592728018928_H
#include "data/header.h"
#include "data/kernelenum.h"
#include <Qt3DCore/QEntity>

namespace qtuser_3d
{
	class SceneOperateMode;
	class FacePicker;
}

namespace creative_kernel
{
	BASIC_KERNEL_API bool isUsingDefaultScene();
	BASIC_KERNEL_API void useCustomScene();
	BASIC_KERNEL_API void useDefaultScene();

	BASIC_KERNEL_API void setVisOperationMode(qtuser_3d::SceneOperateMode* operateMode);
	BASIC_KERNEL_API qtuser_3d::SceneOperateMode* visOperationMode();
	BASIC_KERNEL_API void visShowCustom(Qt3DCore::QEntity* entity);
	BASIC_KERNEL_API void visHideCustom(Qt3DCore::QEntity* entity);
	BASIC_KERNEL_API void visShow(Qt3DCore::QEntity* entity);
	BASIC_KERNEL_API void visHide(Qt3DCore::QEntity* entity);
	BASIC_KERNEL_API void enableVisHandlers(bool enabled);

	BASIC_KERNEL_API void delayCapture(int msec);
	BASIC_KERNEL_API void requestVisUpdate(bool capture = false);
	BASIC_KERNEL_API void requestVisPickUpdate(bool sync);
	BASIC_KERNEL_API qtuser_3d::FacePicker* visPickSource();
	BASIC_KERNEL_API bool visPick(const QPoint& point, int* faceID);

	BASIC_KERNEL_API bool shouldMultipleSelect();
	BASIC_KERNEL_API void showRectangleSelector(const QRect& rect);
	BASIC_KERNEL_API void dismissRectangleSelector();
	BASIC_KERNEL_API bool didSelectAnyEntity(const QPoint& p);

	BASIC_KERNEL_API void showPrimeTower(float x, float y, float radius);
	BASIC_KERNEL_API void hidePrimeTower();

	BASIC_KERNEL_API QSize surfaceSize();
}
#endif // CREATIVE_KERNEL_VISUALSCENEINTERFACE_1592728018928_H