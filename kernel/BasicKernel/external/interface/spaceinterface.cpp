#include "spaceinterface.h"
#include "kernel/kernel.h"
#include "data/modelspace.h"
#include "data/modeln.h"
#include "interface/reuseableinterface.h"

#include "interface/camerainterface.h"
#include "qtuser3d/camera/screencamera.h"

namespace creative_kernel
{
	ModelSpace* getModelSpace()
	{
		return getKernel()->modelSpace();
	}

	void traceSpace(SpaceTracer* tracer)
	{
		getModelSpace()->addSpaceTracer(tracer);
	}

	void unTraceSpace(SpaceTracer* tracer)
	{
		getModelSpace()->removeSpaceTracer(tracer);
	}

	Qt3DCore::QEntity* spaceEntity()
	{
		return getKernel()->modelSpace()->rootEntity();
	}

	void spaceShow(Qt3DCore::QEntity* entity)
	{
		Qt3DCore::QEntity* root = spaceEntity();
		if (entity && entity->parent() != root)
			entity->setParent(root);
	}

	void spaceHide(Qt3DCore::QEntity* entity)
	{
		Qt3DCore::QEntity* root = spaceEntity();
		if (entity && entity->parent() == root)
			entity->setParent((Qt3DCore::QEntity*)nullptr);
	}

	QList<ModelGroup*> modelGroups()
	{
		ModelSpace* space = getKernel()->modelSpace();
		return space->modelGroups();
	}

	QList<ModelN*> modelns()
	{
		return getKernel()->modelSpace()->modelns();
	}

	QString mainModelName()
	{
		return getKernel()->modelSpace()->mainModelName();
	}

	int modelNNum()
	{
		return getKernel()->modelSpace()->modelNNum();
	}

	bool haveModelN()
	{
		return getKernel()->modelSpace()->haveModelN();
	}

	bool haveModelsOutPlatform()
	{
		return getKernel()->modelSpace()->haveModelsOutPlatform();
	}

	QList<FDMSupportGroup*> fdmSuppportGroups()
	{
		QList<ModelN*> models = modelns();
		QList<FDMSupportGroup*> supportsGroups;
		for (ModelN* model : models)
			supportsGroups << model->fdmSupport();
		return supportsGroups;
	}

	bool spaceHasFDMSupport()
	{
		bool result = false;
		QList<ModelGroup*> groups = modelGroups();
		for (ModelGroup* group : groups)
		{
			QList<ModelN*> models = group->models();
			for (ModelN* model : models)
			{
				if (model->hasFDMSupport())
				{
					model->setFDMSuooprt(true);
					result = true;
					break;
				}
			}

			if (result)
				break;
		}

		return result;
	}

	void spaceCancelFDMSupport()
	{
		QList<ModelGroup*> groups = modelGroups();
		for (ModelGroup* group : groups)
		{
			QList<ModelN*> models = group->models();
			for (ModelN* model : models)
			{
				if (model->hasFDMSupport())
				{
					model->setFDMSuooprt(false);
				}
			}
		}
	}

	bool haveSupports(const QList<ModelN*>& models)
	{
		return getKernel()->modelSpace()->haveSupports(models);
	}

	void deleteSupports(QList<ModelN*>& models)
	{
		getKernel()->modelSpace()->deleteSupports(models);
	}

	qtuser_3d::Box3D baseBoundingBox()
	{
		ModelSpace* space = getKernel()->modelSpace();
		return space->baseBoundingBox();
	}

	qtuser_3d::Box3D sceneBoundingBox()
	{
		ModelSpace* space = getKernel()->modelSpace();
		return space->sceneBoundingBox();
	}

	void setBaseBoundingBox(const qtuser_3d::Box3D& box)
	{
		ModelSpace* space = getKernel()->modelSpace();
		return space->setBaseBoundingBox(box);
	}

	void onModelSceneChanged()
	{
		ModelSpace* space = getKernel()->modelSpace();
		return space->onModelSceneChanged();
	}

	int checkModelRange()
	{
		ModelSpace* space = getKernel()->modelSpace();

		qtuser_3d::Box3D box = space->sceneBoundingBox();
		setModelEffectBottomZ(box.min.z());

		onModelSceneChanged();

		return space->checkModelRange();
	}

	int checkBedRange()
	{
		ModelSpace* space = getKernel()->modelSpace();
		return space->checkBedRange();
	}

	void dirtyModelSpace()
	{
		getKernel()->modelSpace()->setSpaceDirty(true);
	}

	void clearModelSpaceDrity()
	{
		getKernel()->modelSpace()->setSpaceDirty(false);
	}

	bool modelSpaceDirty()
	{
		return getKernel()->modelSpace()->spaceDirty();
	}
}