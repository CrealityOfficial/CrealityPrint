#include "spacemodifycommand.h"
#include "internal/data/_modelinterface.h"
#include "data/modeln.h"

namespace creative_kernel
{
	SpaceModifyCommand::SpaceModifyCommand(Qt3DCore::QNode* parent)
		:QNode(parent)
	{
	}
	
	SpaceModifyCommand::~SpaceModifyCommand()
	{
	}

	void SpaceModifyCommand::undo()
	{
		deal(false);
	}

	void SpaceModifyCommand::redo()
	{
		deal(true);
	}

	void SpaceModifyCommand::setModels(const QList<ModelN*>& models)
	{
		m_modelList = models;
	}

	void SpaceModifyCommand::setRemoveModels(const QList<ModelN*>& models)
	{
		m_modelRemoveList = models;
	}

	void SpaceModifyCommand::deal(bool redo)
	{
		QList<ModelN*> removes = redo ? m_modelRemoveList : m_modelList;
		QList<ModelN*> adds = redo ? m_modelList : m_modelRemoveList;
		
		_modifySpace(removes, adds);

		for (ModelN* model : removes)
			model->setParent(this);
	}
}