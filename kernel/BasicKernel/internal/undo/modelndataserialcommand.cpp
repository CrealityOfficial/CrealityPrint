#include "modelndataserialcommand.h"
#include "data/modeln.h"
#include "cxkernel/data/modelndataserial.h"
#include "interface/modelinterface.h"
#include <QDateTime>
#include <QThread>

namespace creative_kernel
{
	ModelNDataSerialCommand::ModelNDataSerialCommand(QList<ModelN*>& models, QList<cxkernel::ModelNDataPtr>& modelnDatas, bool resetModel)
		: m_modelnDatas(modelnDatas)
		, m_resetModel(resetModel)
	{
		m_modelSerialNameList.clear();
		for (ModelN* m : models)
		{
			m_modelSerialNameList << m->getSerialName();
		}
	}
	
	ModelNDataSerialCommand::~ModelNDataSerialCommand()
	{
		m_savedSerialInfos.clear();
		m_modelSerialNameList.clear();
		m_modelnDatas.clear();
	}

	void ModelNDataSerialCommand::undo()
	{
		QList<ModelN*> theModels;
		QList<cxkernel::ModelNDataPtr> serialdatas;
		theModels = getModelnsBySerialName(m_modelSerialNameList);

		if(!m_savedSerialInfos.isEmpty())
			recoverModelNDataFromSerialData(m_savedSerialInfos, serialdatas);

		if (serialdatas.size() > 0)
		{

			serializeRemovedModelNDatas(theModels, m_savedSerialInfos);

			if (theModels.size() == serialdatas.size())
			{
				for (int i = 0; i < theModels.size(); i++)
				{
					ModelN* modelPtr = theModels.at(i);
					modelPtr->setData(serialdatas.at(i));

					//if (m_resetModel)
					//{
					//	theModels.at(i)->setUnUniformScaled(false);
					//}
				}
			}
			else
			{
				qWarning() << Q_FUNC_INFO << "theModels.size() != serialdatas.size(), exception!!!";
			}

		}

	}

	void ModelNDataSerialCommand::redo()
	{
		QList<ModelN*> theModels = getModelnsBySerialName(m_modelSerialNameList);

		if (!m_savedSerialInfos.isEmpty())
		{
			recoverModelNDataFromSerialData(m_savedSerialInfos, m_modelnDatas);
		}

		serializeRemovedModelNDatas(theModels, m_savedSerialInfos);

		if (theModels.size() == m_modelnDatas.size())
		{
			ModelN* modelPtr = nullptr;
			for (int i = 0; i < theModels.size(); i++)
			{
				modelPtr = theModels.at(i);

				modelPtr->setData(m_modelnDatas.at(i));

				if (m_resetModel)
				{
					modelPtr->resetLocalQuaternion(false);
					modelPtr->resetLocalScale(true);
				}

			}
		}
		else
		{
			qWarning() << Q_FUNC_INFO << "theModels.size() != m_modelnDatas.size(), exception!!!";
		}

		// this call is very important for the ModelNDataPtr  dereference
		m_modelnDatas.clear();

	}

	void ModelNDataSerialCommand::serializeRemovedModelNDatas(const QList<ModelN*>& theModels, QList<MD_SAVED_INFO>& savedInfos)
	{
		savedInfos.clear();

		if (theModels.isEmpty())
			return;

		cxkernel::ModelNDataSerial dataSerializer;

		for (ModelN* model : theModels)
		{
			if (model->modelNData())
			{
				MD_SAVED_INFO savedInfo;
				savedInfo.inputName = model->modelNData()->input.name;
				savedInfo.serialName = model->getSerialName();

				dataSerializer.setData(model->modelNData());
				dataSerializer.save(savedInfo.serialName, nullptr);

				savedInfos.append(savedInfo);

			}
		}

	}

	void ModelNDataSerialCommand::recoverModelNDataFromSerialData(const QList<MD_SAVED_INFO>& savedSerialInfos, QList<cxkernel::ModelNDataPtr>& serialdatas)
	{
		
		if (savedSerialInfos.isEmpty())
			return;

		serialdatas.clear();
		cxkernel::ModelNDataSerial dataSerializer;

		for (const MD_SAVED_INFO& savedInfo : savedSerialInfos)
		{
			dataSerializer.setData(nullptr);
			dataSerializer.load(savedInfo.serialName, nullptr);
			if (dataSerializer.getData())
			{
				dataSerializer.getData()->input.name = savedInfo.inputName;
				serialdatas.append(dataSerializer.getData());
			}
			
		}

	}
}