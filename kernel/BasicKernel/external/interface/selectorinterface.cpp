#include "selectorinterface.h"
#include "external/kernel/kernel.h"
#include "data/modeln.h"
#include "qtuser3d/module/pickable.h"
#include "kernel/modelnselector.h"

#include "interface/visualsceneinterface.h"
#include "interface/camerainterface.h"
#include "interface/spaceinterface.h"
#include "interface/modelgroupinterface.h"

using namespace qtuser_3d;
namespace creative_kernel
{
	bool wipeTowerSelected() 
	{ 
		return getKernel()->modelSelector()->wipeTowerSelected(); 
	}

	void addModelNSelectorTracer(ModelNSelectorTracer* tracer)
	{
		getKernel()->modelSelector()->addModelNSelectorTracer(tracer);
	}

	void removeModelNSelectorTracer(ModelNSelectorTracer* tracer)
	{
		getKernel()->modelSelector()->removeModelNSelectorTracer(tracer);
	}

	void notifyPickableChanged(qtuser_3d::Pickable* pickable)
	{
		getKernel()->modelSelector()->changed(pickable);
	}

	Printer* selectedPlate()
	{
		return getKernel()->modelSelector()->selectedPlate();
	}

	QList<ModelN*> selectionms()
	{
		return getKernel()->modelSelector()->selectionms();
	}

	QList<ModelN*> selectionms(ModelNType type)
	{
		return getKernel()->modelSelector()->selectionms(type);
	}

	QList<ModelN*> selectedParts()
	{
		return getKernel()->modelSelector()->selectedParts();
	}

	QList<ModelN*> unselectionms() {
		return getKernel()->modelSelector()->unselectionms();
	}

	QList<ModelGroup*> selectedGroups()
	{
		return getKernel()->modelSelector()->selectedGroups();
	}

	QList<QList<ModelN*>> selectedModelnsList()
	{
		return getKernel()->modelSelector()->selectedModelnsList();
	}

	qtuser_3d::Box3D selectionsBoundingbox()
	{
		return getKernel()->modelSelector()->selectionsBoundingbox();
	}

	int selectedPartsNum()
	{
		return getKernel()->modelSelector()->selectedPartsNum();
	}

	int selectedGroupsNum()
	{
		return getKernel()->modelSelector()->selectedGroupsNum();
	}

	void tracePickable(Pickable* entity)
	{
		getKernel()->modelSelector()->addPickable(entity);
	}

	void unTracePickable(Pickable* entity)
	{
		getKernel()->modelSelector()->removePickable(entity);
	}

	void addPickables(QList<qtuser_3d::Pickable*> pickables)
	{
		getKernel()->modelSelector()->addPickables(pickables);
	}

	void removePickables(QList<qtuser_3d::Pickable*> pickables)
	{
		getKernel()->modelSelector()->removePickables(pickables);
	}

	Pickable* checkPickable(const QPoint& point, int* primitiveID)
	{
		return getKernel()->modelSelector()->check(point, primitiveID);
	}

	ModelN* checkPickModel(const QPoint& point, QVector3D& position, QVector3D& normal, int* primitiveID)
	{
		return getKernel()->modelSelector()->checkPickModel(point, position, normal, primitiveID);
	}

	ModelN* checkPickModel(const QPoint& point)
	{
		return getKernel()->modelSelector()->checkPickModel(point);
	}

	// the normal is returned after applyed by (normal matrix)
	ModelN* checkPickModelEx(const QPoint& point, QVector3D& position, QVector3D& normal, int* primitiveID)
	{
		return getKernel()->modelSelector()->checkPickModelEx(point, position, normal, primitiveID);
	}

	bool pointHover(QHoverEvent* event)
	{
		return getKernel()->modelSelector()->pointHover(event);
	}

	void pointSelect(QMouseEvent* event)
	{
		getKernel()->modelSelector()->pointSelect(event);
	}

	void areaSelect(const QRect& rect, QMouseEvent* event)
	{
		getKernel()->modelSelector()->areaSelect(rect, event);
	}

	void selectAllModelGroup()
	{
		getKernel()->modelSelector()->selectAllModelGroup();
	}

	void ctrlASelectAll()
	{
		getKernel()->modelSelector()->ctrlASelectAll();
	}

	void updateFaceBases()
	{
		getKernel()->modelSelector()->updateFaceBases();
	}

	void removeSelections()
	{
		getKernel()->modelSelector()->removeSelections();
	}

	void mirrorXSelections(bool reversible)
	{
		getKernel()->modelSelector()->mirrorXSelections(reversible);
	}

	void mirrorYSelections(bool reversible)
	{
		getKernel()->modelSelector()->mirrorYSelections(reversible);
	}

	void mirrorZSelections(bool reversible)
	{
		getKernel()->modelSelector()->mirrorZSelections(reversible);
	}

	void mergeSelectionsGroup()
	{
		getKernel()->modelSelector()->mergeSelectionsGroup();
	}

	void splitSelectionsGroup2Objects()
	{
		getKernel()->modelSelector()->splitSelectionsGroup2Objects();
	}

	void splitSelectionsModels2Parts()
	{
		getKernel()->modelSelector()->splitSelectionsModels2Parts();
	}

	void cloneSelections(int num)
	{
		getKernel()->modelSelector()->cloneSelections(num);
	}

	void onCopySelections()
	{
		getKernel()->modelSelector()->onCopySelections();
	}

	bool canPasteEnabled()
	{
	  return getKernel()->modelSelector()->canPasteEnabled();
	}

	void onPasteSelections()
	{
		getKernel()->modelSelector()->onPasteSelections();
	}

	void selectGroup(ModelGroup* group, bool append)
	{
		getKernel()->modelSelector()->selectGroup(group, append);
	}

	void selectGroups(const QList<ModelGroup*>& groups)
	{
		getKernel()->modelSelector()->selectGroups(groups);
	}

	void selectModelN(ModelN* model, bool append, bool force)
	{
		getKernel()->modelSelector()->selectModelN(model, append, force);
	}

	void unselectGroups(const QList<ModelGroup*>& groups)
	{
		getKernel()->modelSelector()->unselectGroups(groups);
	}

	void unselectModels(const QList<ModelN*>& models)
	{
		getKernel()->modelSelector()->unselectModels(models);
	}

	void unselectAll()
	{
		getKernel()->modelSelector()->unselectAll();
	}
}
