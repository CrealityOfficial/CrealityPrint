#include "modelnselector.h"
#include "qtuser3d/module/pickable.h"
#include "qtuser3d/module/facepicker.h"
#include "qtuser3d/module/pickableselecttracer.h"
#include "qtuserqml/plugin/toolcommandcenter.h"
#include "internal/tool/scalemode.h"
#include "data/modeln.h"
#include "kernel/kernelui.h"
using namespace qtuser_3d;
namespace creative_kernel
{
	ModelNSelector::ModelNSelector(QObject* parent)
		: Selector(parent)
	{
		addTracer(this);
	}
	
	ModelNSelector::~ModelNSelector()
	{
	}

	void ModelNSelector::sizeChanged()
	{
		emit selectedNumChanged();
	}

	QList<ModelN*> ModelNSelector::selectionms()
	{
		QList<ModelN*> models;
		QList<qtuser_3d::Pickable*> selections = selectionmPickables();
		for (Pickable* pickable : selections)
		{
			ModelN* m = qobject_cast<ModelN*>(pickable);
			if (m)
				models << m;
		}
		return models;
	}

	int ModelNSelector::selectedNum()
	{
		return selectionms().size();
	}

	QVector3D ModelNSelector::selectionmsSize()
	{
		QList<ModelN*> models = selectionms();
		qtuser_3d::Box3D box;
		for(ModelN* model : models)
			box += model->globalSpaceBox();
		return box.size();
	}

	void ModelNSelector::onSelectionsChanged()
	{
		emit selectedNumChanged();
	}

	void ModelNSelector::selectChanged(qtuser_3d::Pickable* pickable)
	{
	}
}