#include "spaceinterface.h"
#include "kernel/kernel.h"
#include "data/modelspace.h"
#include "data/modeln.h"
#include "interface/reuseableinterface.h"

#include "interface/camerainterface.h"
#include "qtuser3d/camera/screencamera.h"
#include "data/modelspaceundo.h"
#include "interface/modelinterface.h"
#include "interface/printerinterface.h"
#include "interface/projectinterface.h"
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
		if (!entity->isEnabled())
			entity->setEnabled(true);
	}

	void spaceHide(Qt3DCore::QEntity* entity)
	{
		Qt3DCore::QEntity* root = spaceEntity();
		if (entity && entity->parent() != root)
			entity->setParent(root);

		if (entity->isEnabled())
			entity->setEnabled(false);

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

	BASIC_KERNEL_API bool modelOutPlatform(ModelN* amodel)
	{
		return getKernel()->modelSpace()->modelOutPlatform(amodel);
	}

	ModelN* findModelBySerialName(const QString& serialName)
	{
		for (auto m : modelns())
		{
			if (m->getSerialName() == serialName)
				return m;
		}
		return NULL;
	}

	qtuser_3d::Box3D modelsBox(const QList<ModelN*>& models)
	{
		qtuser_3d::Box3D box;
		for (ModelN* model : models)
			box += model->globalSpaceBox();
		return box;
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

		/*qtuser_3d::Box3D box = space->sceneBoundingBox();
		setModelEffectBottomZ(box.min.z());*/

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

	void onModelMeshLoadStart(int iMeshNum)
	{
		getModelSpace()->modelMeshLoadStarted(iMeshNum);
	}
	void clearProjectCache(bool clearModel)
	{
		if (clearModel)
		{
			resetPrinterNum();
			removeAllModel(false);
		}	
		getKernel()->modelSpaceUndo()->clear();
		clearProjectSpaceDrity();
	}

	bool spaceHasWipeTower()
	{
		QSet<int> allIndexs;

		for (auto m : modelns())
		{
			allIndexs.unite(m->data()->colorIndexs);
		}

		return allIndexs.size() >= 2;
	}
}