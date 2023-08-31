#include "selectorinterface.h"
#include "external/kernel/kernel.h"
#include "data/modeln.h"
#include "qtuser3d/module/pickable.h"
#include "kernel/modelnselector.h"

#include "interface/visualsceneinterface.h"
#include "interface/camerainterface.h"
#include "interface/spaceinterface.h"

using namespace qtuser_3d;
namespace creative_kernel
{
	void addSelectTracer(SelectorTracer* tracer)
	{
		getKernel()->modelSelector()->addTracer(tracer);
	}

	void removeSelectorTracer(SelectorTracer* tracer)
	{
		getKernel()->modelSelector()->removeTracer(tracer);
	}

	void disableReverseSelect(bool disable)
	{
		getKernel()->modelSelector()->disableReverseSelect(disable);
	}

	QList<ModelN*> selectionms()
	{
		return getKernel()->modelSelector()->selectionms();
	}

	QList<ModelN*> unselectionms() {
		QList<ModelN*> selected_model_list = selectionms();
		QList<ModelN*> unselect_model_list;

		for (auto* model : modelns()) {
			if (!selected_model_list.contains(model)) {
				unselect_model_list << model;
			}
		}

		return unselect_model_list;
	}

	void tracePickable(Pickable* entity)
	{
		getKernel()->modelSelector()->addPickable(entity);
	}

	void unTracePickable(Pickable* entity)
	{
		getKernel()->modelSelector()->removePickable(entity);
	}

	Pickable* checkPickable(const QPoint& point, int* primitiveID)
	{
		return getKernel()->modelSelector()->check(point, primitiveID);
	}

	ModelN* checkPickModel(const QPoint& point, QVector3D& position, QVector3D& normal, int* faceID)
	{
		int primitiveID = -1;
		Pickable* pickable = checkPickable(point, &primitiveID);
		ModelN* model = qobject_cast<ModelN*>(pickable);
		if (model)
		{
			model->rayCheck(primitiveID, visRay(point), position, &normal);
			if (model->isFanZhuan())
			{
				normal = -normal;
			}
		}
		if (faceID)
		{
			*faceID = primitiveID;
		}
		return model;
	}

	ModelN* rclickModelN(const QPoint& point)
	{
		int primitiveID = -1;
		Pickable* pickable = checkPickable(point, &primitiveID);

		if (pickable != nullptr)
		{
			QString qname = pickable->metaObject()->className();
			if (qname.indexOf("ModelN") == -1)
			{
				return nullptr;
			}
		}
		return (ModelN*)pickable;
	}

	void selectedPickableInfoChanged(qtuser_3d::Pickable* pickable)
	{
		getKernel()->modelSelector()->changed(pickable);
	}

	bool hoverVis(const QPoint& point)
	{
		return getKernel()->modelSelector()->hover(point);
	}

	void selectVis(const QPoint& point, bool append)
	{
		getKernel()->modelSelector()->select(point, append);
		requestVisUpdate();
	}

	void selectAll()
	{
		getKernel()->modelSelector()->selectAll();
		requestVisUpdate();
	}

	void unselectAll()
	{
		getKernel()->modelSelector()->selectNone();
		requestVisUpdate();
	}

	void selectArea(const QRect& rect)
	{
		getKernel()->modelSelector()->selectRect(rect);
	}
		
	ModelNSelector* getModelSelect()
	{
		return getKernel()->modelSelector();
	}

	void appendSelect(qtuser_3d::Pickable* pickable)
	{
		getKernel()->modelSelector()->appendSelect(pickable);
		requestVisUpdate();
	}

	void appendSelects(const QList<qtuser_3d::Pickable*> pickables)
	{
		getKernel()->modelSelector()->appendSelects(pickables);
		requestVisUpdate();
	}

	void unselectOne(qtuser_3d::Pickable* pickable)
	{
		getKernel()->modelSelector()->unselectOne(pickable);
		requestVisUpdate();
	}

	void unselectMore(const QList<qtuser_3d::Pickable*> pickables)
	{
		getKernel()->modelSelector()->unselectMore(pickables);
		requestVisUpdate();
	}

	void selectOne(qtuser_3d::Pickable* pickable)
	{
		getKernel()->modelSelector()->selectOne(pickable);
		requestVisUpdate();
	}

	void selectMore(const QList<qtuser_3d::Pickable*> pickables)
	{
		getKernel()->modelSelector()->selectMore(pickables);
		requestVisUpdate();
	}

	void updateFaceBases()
	{
		getKernel()->modelSelector()->updateFaceBases();
	}
}
