#include "modelgroup.h"

#include "data/modeln.h"
#include "interface/visualsceneinterface.h"
#include "interface/selectorinterface.h"
#include "interface/appsettinginterface.h"

#include "us/usettings.h"

namespace creative_kernel
{
	ModelGroup::ModelGroup(QObject* parent)
		:Node3D(parent)
	{
		m_parentMatrixDirty = false;
		m_localMatrixDirty = false;
		m_setting = new us::USettings(this);
	}
	
	ModelGroup::~ModelGroup()
	{
	}

	void ModelGroup::addModel(ModelN* model)
	{
		if (!model) return;

		model->setParent(this);
		model->updateMatrix();
		model->setVisibility(true);
	}

	void ModelGroup::removeModel(ModelN* model)
	{
		if (!model) return;
		
		model->setParent(nullptr);
		model->setVisibility(false);
	}

	void ModelGroup::setChildrenVisibility(bool visibility)
	{
		QList<ModelN*> modelns = models();
		for (ModelN* modeln : modelns)
		{
			modeln->setVisibility(visibility);
		}
	}

	QList<ModelN*> ModelGroup::models()
	{
		QList<ModelN*> ml = findChildren<ModelN*>(QString(), Qt::FindDirectChildrenOnly);
		return ml;
	}

	us::USettings* ModelGroup::setting()
	{
		return m_setting;
	}
}