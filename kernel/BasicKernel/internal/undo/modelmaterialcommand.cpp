#include "modelmaterialcommand.h"
#include "data/modeln.h"
#include "interface/spaceinterface.h"

namespace creative_kernel
{
	ModelMaterialCommand::ModelMaterialCommand(QList<ModelN*>& models, int newMaterial)
	{
		for (ModelN* model : models)
		{
			m_serialNames << model->getSerialName();
			m_oldMaterials << model->defaultColorIndex();
		}
		m_newMaterial = newMaterial;
	}
	
	ModelMaterialCommand::~ModelMaterialCommand()
	{

	}

	void ModelMaterialCommand::undo()
	{
		for (int i = 0, count = m_serialNames.count(); i < count; ++i)
		{
			QString name = m_serialNames[i];
			int material = m_oldMaterials[i];
			ModelN* model = findModelBySerialName(name);
			if (model)
			{
				model->setDefaultColorIndex(material);	
			}
		}
	}

	void ModelMaterialCommand::redo()
	{
		for (QString name : m_serialNames)
		{
			ModelN* model = findModelBySerialName(name);
			if (model)
			{
				model->setDefaultColorIndex(m_newMaterial);	
			}
		}

	}
}