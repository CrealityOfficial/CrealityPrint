#ifndef CREATIVE_KERNEL_VISUALSCENEINTERFACE_1592728018928_H
#define CREATIVE_KERNEL_VISUALSCENEINTERFACE_1592728018928_H
#include "data/header.h"
#include "data/kernelenum.h"
#include <Qt3DCore/QEntity>
#include "qtuser3d/framegraph/rendercaptor.h"

namespace qtuser_3d
{
	class SceneOperateMode;
	class FacePicker;
	class Box3D;
}

namespace creative_kernel
{
	BASIC_KERNEL_API Qt3DCore::QEntity* spaceEntity();
	BASIC_KERNEL_API void spaceShow(Qt3DCore::QEntity* entity);
	BASIC_KERNEL_API void spaceHide(Qt3DCore::QEntity* entity);

	BASIC_KERNEL_API void requestCapture(Qt3DRender::QCamera* camera, QObject* receiver, qtuser_3d::RenderCaptor::ReceiverHandleReplyFunc func);

	BASIC_KERNEL_API bool isUsingDefaultScene();
	BASIC_KERNEL_API bool isUsingPreviewScene();

	BASIC_KERNEL_API void usePrepareScene();
	BASIC_KERNEL_API void usePreviewScene();
	BASIC_KERNEL_API void useCustomScene();
	BASIC_KERNEL_API void setPreviewEnabled(bool enabled);

	BASIC_KERNEL_API void clearRenderRequestors();
	BASIC_KERNEL_API void requestRender(QObject* requestor);
	BASIC_KERNEL_API void endRequestRender(QObject* requestor);

	BASIC_KERNEL_API void setVisOperationMode(qtuser_3d::SceneOperateMode* operateMode);
	BASIC_KERNEL_API qtuser_3d::SceneOperateMode* visOperationMode();
	BASIC_KERNEL_API void visShowCustom(Qt3DCore::QEntity* entity);
	BASIC_KERNEL_API void visHideCustom(Qt3DCore::QEntity* entity);
	BASIC_KERNEL_API void visShow(Qt3DCore::QEntity* entity);
	BASIC_KERNEL_API void visHide(Qt3DCore::QEntity* entity);
	BASIC_KERNEL_API void enableVisHandlers(bool enabled);

	BASIC_KERNEL_API void setBoxEnabled(bool enabled);
	BASIC_KERNEL_API void forceHideBox(bool enabled);

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

	BASIC_KERNEL_API void updateSceneSelectionsGlobalBox(const qtuser_3d::Box3D& box);

	BASIC_KERNEL_API void setModelVisualMode(ModelVisualMode mode);
	BASIC_KERNEL_API ModelVisualMode modelVisualMode();
}
#endif // CREATIVE_KERNEL_VISUALSCENEINTERFACE_1592728018928_H
