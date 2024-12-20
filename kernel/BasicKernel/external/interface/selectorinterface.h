#ifndef CREATIVE_KERNEL_SELECTORINTERFACE_1592731042119_H
#define CREATIVE_KERNEL_SELECTORINTERFACE_1592731042119_H
#include "basickernelexport.h"
#include "data/interface.h"
#include <QtGui/QVector3D>
#include <QtCore/QPoint>
#include <QtGui/qevent.h>

namespace qtuser_3d
{
	class Pickable;
}

namespace creative_kernel
{
	class ModelN;
	class ModelGroup;
	class ModelNSelector;
	class Printer;

	BASIC_KERNEL_API bool wipeTowerSelected();

	BASIC_KERNEL_API void addModelNSelectorTracer(ModelNSelectorTracer* tracer);
	BASIC_KERNEL_API void removeModelNSelectorTracer(ModelNSelectorTracer* tracer);

	BASIC_KERNEL_API void notifyPickableChanged(qtuser_3d::Pickable* pickable);

	BASIC_KERNEL_API Printer* selectedPlate();
	BASIC_KERNEL_API QList<ModelN*> selectionms();
	BASIC_KERNEL_API QList<ModelN*> selectionms(ModelNType type);
	BASIC_KERNEL_API QList<ModelN*> selectedParts();
	BASIC_KERNEL_API QList<ModelN*> unselectionms();
	BASIC_KERNEL_API QList<ModelGroup*> selectedGroups();
	BASIC_KERNEL_API QList<QList<ModelN*>> selectedModelnsList();
	BASIC_KERNEL_API qtuser_3d::Box3D selectionsBoundingbox();

	BASIC_KERNEL_API int selectedPartsNum();
	BASIC_KERNEL_API int selectedGroupsNum();

	BASIC_KERNEL_API void tracePickable(qtuser_3d::Pickable* pickable);
	BASIC_KERNEL_API void unTracePickable(qtuser_3d::Pickable* pickable);
	BASIC_KERNEL_API void addPickables(QList<qtuser_3d::Pickable*> pickables);
	BASIC_KERNEL_API void removePickables(QList<qtuser_3d::Pickable*> pickables);

	BASIC_KERNEL_API qtuser_3d::Pickable* checkPickable(const QPoint& point, int* primitiveID);
	BASIC_KERNEL_API ModelN* checkPickModel(const QPoint& point, QVector3D& position, QVector3D& normal, int* primitiveID = nullptr);
	BASIC_KERNEL_API ModelN* checkPickModel(const QPoint& point);

	// the normal is returned after applyed by (normal matrix)
	BASIC_KERNEL_API ModelN* checkPickModelEx(const QPoint& point, QVector3D& position, QVector3D& normal, int* primitiveID = nullptr);

	BASIC_KERNEL_API bool pointHover(QHoverEvent* event);
	BASIC_KERNEL_API void pointSelect(QMouseEvent* event);
	BASIC_KERNEL_API void areaSelect(const QRect& rect, QMouseEvent* event);
	BASIC_KERNEL_API void selectAllModelGroup();
	BASIC_KERNEL_API void ctrlASelectAll();
	BASIC_KERNEL_API void selectGroup(ModelGroup* group, bool append = false);
	BASIC_KERNEL_API void selectGroups(const QList<ModelGroup*>& groups);
	BASIC_KERNEL_API void selectModelN(ModelN* model, bool append = false, bool force = false);
	BASIC_KERNEL_API void unselectGroups(const QList<ModelGroup*>& groups);
	BASIC_KERNEL_API void unselectModels(const QList<ModelN*>& models);
	BASIC_KERNEL_API void unselectAll();

	BASIC_KERNEL_API void updateFaceBases();

	BASIC_KERNEL_API void removeSelections();
	BASIC_KERNEL_API void mirrorXSelections(bool reversible = false);
	BASIC_KERNEL_API void mirrorYSelections(bool reversible = false);
	BASIC_KERNEL_API void mirrorZSelections(bool reversible = false);

	BASIC_KERNEL_API void cloneSelections(int num);
	BASIC_KERNEL_API void onCopySelections();
	BASIC_KERNEL_API bool canPasteEnabled();
	BASIC_KERNEL_API void onPasteSelections();

	BASIC_KERNEL_API void mergeSelectionsGroup();
	BASIC_KERNEL_API void splitSelectionsGroup2Objects();
	BASIC_KERNEL_API void splitSelectionsModels2Parts();
}
#endif // CREATIVE_KERNEL_SELECTORINTERFACE_1592731042119_H
