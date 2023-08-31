#ifndef CREATIVE_KERNEL_SPACEINTERFACE_1592880998363_H
#define CREATIVE_KERNEL_SPACEINTERFACE_1592880998363_H
#include "basickernelexport.h"
#include <Qt3DCore/QEntity>
#include "data/modelspace.h"
#include "data/modelgroup.h"

class SpaceTracer;
namespace creative_kernel
{
	class FDMSupportGroup;
	BASIC_KERNEL_API Qt3DCore::QEntity* spaceEntity();
	BASIC_KERNEL_API void spaceShow(Qt3DCore::QEntity* entity);
	BASIC_KERNEL_API void spaceHide(Qt3DCore::QEntity* entity);

	BASIC_KERNEL_API ModelSpace* getModelSpace();
	BASIC_KERNEL_API void traceSpace(SpaceTracer* tracer);
	BASIC_KERNEL_API void unTraceSpace(SpaceTracer* tracer);

	BASIC_KERNEL_API QList<ModelGroup*> modelGroups();
	BASIC_KERNEL_API QList<ModelN*> modelns();
	BASIC_KERNEL_API QString mainModelName();
	BASIC_KERNEL_API int modelNNum();
	BASIC_KERNEL_API bool haveModelN();
	BASIC_KERNEL_API bool haveModelsOutPlatform();

	BASIC_KERNEL_API QList<FDMSupportGroup*> fdmSuppportGroups();
	BASIC_KERNEL_API bool spaceHasFDMSupport();
	BASIC_KERNEL_API void spaceCancelFDMSupport();
	BASIC_KERNEL_API bool haveSupports(const QList<ModelN*>& models);
	BASIC_KERNEL_API void deleteSupports(QList<ModelN*>& models);


	BASIC_KERNEL_API qtuser_3d::Box3D baseBoundingBox();
	BASIC_KERNEL_API qtuser_3d::Box3D sceneBoundingBox();      // changed when scene modified , trait it again

	BASIC_KERNEL_API void setBaseBoundingBox(const qtuser_3d::Box3D& box);

	BASIC_KERNEL_API void onModelSceneChanged();

	BASIC_KERNEL_API int checkModelRange();

	BASIC_KERNEL_API int checkBedRange();

	BASIC_KERNEL_API void dirtyModelSpace();
	BASIC_KERNEL_API void clearModelSpaceDrity();
	BASIC_KERNEL_API bool modelSpaceDirty();
}
#endif // CREATIVE_KERNEL_SPACEINTERFACE_1592880998363_H