#ifndef CREATIVE_KERNEL_SELECTORINTERFACE_1592731042119_H
#define CREATIVE_KERNEL_SELECTORINTERFACE_1592731042119_H
#include "basickernelexport.h"
#include <QtGui/QVector3D>
#include <QtCore/QPoint>
#include <Qt3DCore/qnode.h>

namespace qtuser_3d
{
	class Pickable;
	class SelectorTracer;
}

namespace creative_kernel
{
	class ModelN;
	class ModelNSelector;
	BASIC_KERNEL_API void addSelectTracer(qtuser_3d::SelectorTracer* tracer);
	BASIC_KERNEL_API void removeSelectorTracer(qtuser_3d::SelectorTracer* tracer);
	BASIC_KERNEL_API void disableReverseSelect(bool disable);

	BASIC_KERNEL_API QList<ModelN*> selectionms();
	BASIC_KERNEL_API QList<ModelN*> unselectionms();

	BASIC_KERNEL_API void tracePickable(qtuser_3d::Pickable* pickable);
	BASIC_KERNEL_API void unTracePickable(qtuser_3d::Pickable* pickable);
	BASIC_KERNEL_API qtuser_3d::Pickable* checkPickable(const QPoint& point, int* primitiveID);
	BASIC_KERNEL_API ModelN* checkPickModel(const QPoint& point, QVector3D& position, QVector3D& normal, int* primitiveID = nullptr);

	BASIC_KERNEL_API ModelN* rclickModelN(const QPoint& point);

	BASIC_KERNEL_API void selectedPickableInfoChanged(qtuser_3d::Pickable* pickable);
	BASIC_KERNEL_API bool hoverVis(const QPoint& point);
	BASIC_KERNEL_API void selectVis(const QPoint& point, bool append = false);
	BASIC_KERNEL_API void selectAll();
	BASIC_KERNEL_API void unselectAll();
	BASIC_KERNEL_API void selectArea(const QRect& rect);

    BASIC_KERNEL_API void appendSelect(qtuser_3d::Pickable* pickable);
	BASIC_KERNEL_API void appendSelects(const QList<qtuser_3d::Pickable*> pickables);
	BASIC_KERNEL_API void unselectOne(qtuser_3d::Pickable* pickable);
	BASIC_KERNEL_API void unselectMore(const QList<qtuser_3d::Pickable*> pickables);
	BASIC_KERNEL_API void selectOne(qtuser_3d::Pickable* pickable);
	BASIC_KERNEL_API void selectMore(const QList<qtuser_3d::Pickable*> pickables);
	BASIC_KERNEL_API ModelNSelector* getModelSelect();

	BASIC_KERNEL_API void updateFaceBases();
}
#endif // CREATIVE_KERNEL_SELECTORINTERFACE_1592731042119_H
