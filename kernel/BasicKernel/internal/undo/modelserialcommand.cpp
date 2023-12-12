#include "modelserialcommand.h"
#include "data/modeln.h"
#include "cxkernel/data/modelndataserial.h"
#include "cxkernel/utils/utils.h"
#include "interface/modelinterface.h"
#include <QDateTime>
#include <QThread>
#include "interface/modelinterface.h"

namespace creative_kernel
{
	ModelSerialCommand::ModelSerialCommand(QList<ModelN*>& removes, QList<ModelN*>& adds)
	{
		m_adds = adds;
		m_removes = removes;
		m_firstRedo = true;
		m_serialRemoveNameList.clear();
		for (ModelN* m : removes)
		{
			m_serialRemoveNameList << m->getSerialName();
		}

		m_serialAddNameList.clear();
		for (ModelN* m : adds)
		{
			//for each model added to scene, create a new serialName for that model
			m->setSerialName(cxkernel::getSerializeModelName(m->modelNData()->input.name));

			m_serialAddNameList << m->getSerialName();

			// to prevent generating same serialName if multiple models in adds have the same input name
			QThread::msleep(10);
		}
	}
	
	ModelSerialCommand::~ModelSerialCommand()
	{
		m_savedSerialInfos.clear();
		m_serialRemoveNameList.clear();
		m_serialAddNameList.clear();
	}

	// undo mainly deal with "recover removed model from serialize file, and delete the previous created model"
	void ModelSerialCommand::undo()
	{
		m_removes = getModelnsBySerialName(m_serialAddNameList);

		if(!m_savedSerialInfos.isEmpty())
			recoverModelFromSerialData(m_savedSerialInfos, m_adds);
		else
		{
			m_adds = getModelnsBySerialName(m_serialRemoveNameList);
		}

		
			
			//m_space->batchModels(m_removes, m_adds, false);

			//before the model is removed completely, the modelnData need to be serialized to file
			serializeRemovedModels(m_removes, m_savedSerialInfos);

			modifySpace(m_removes, m_adds, false);
			m_removes.clear();

			if(m_serialAddNameList.isEmpty())
				m_adds.clear();

		

	}

	// redo mainly deal with "remove model completely, and add model to scene"
	void ModelSerialCommand::redo()
	{
		if (!m_savedSerialInfos.isEmpty())
		{
			recoverModelFromSerialData(m_savedSerialInfos, m_adds);
		}
		else if(!m_firstRedo)
		{
			m_adds = getModelnsBySerialName(m_serialAddNameList);
		}

		m_removes = getModelnsBySerialName(m_serialRemoveNameList);

		serializeRemovedModels(m_removes, m_savedSerialInfos);

		
		modifySpace(m_removes, m_adds, false);

		/*for (ModelN* m : m_removes)
		{
			delete m;
			m = nullptr;
		}*/

		m_removes.clear();

		m_firstRedo = false;

	}

	void ModelSerialCommand::serializeRemovedModels(const QList<ModelN*>& removeModels, QList<SAVED_INFO>& savedInfos)
	{
		savedInfos.clear();

		if (removeModels.isEmpty())
			return;

#ifdef DEBUG
		qint64 t1 = QDateTime::currentDateTime().toMSecsSinceEpoch();
#endif // DEBUG 

		cxkernel::ModelNDataSerial dataSerializer;

		for (ModelN* model : removeModels)
		{
			if (model->modelNData())
			{
				SAVED_INFO savedInfo;
				savedInfo.inputName = model->modelNData()->input.name;
				savedInfo.serialName = model->getSerialName();
				savedInfo.localPosition = model->localPosition();
				savedInfo.localScale = model->localScale();
				savedInfo.localQuaternion = model->localQuaternion();
				//savedInfo.isHoneyFilled = model->isHoneyFilled();
				//savedInfo.isHoneyHollowed = model->isHoneyHollowed();

				dataSerializer.setData(model->modelNData());
				dataSerializer.save(savedInfo.serialName, nullptr);

				savedInfos.append(savedInfo);
			}
		}

#ifdef DEBUG
		qint64 t1_1 = QDateTime::currentDateTime().toMSecsSinceEpoch();
		qDebug() << "serializeRemovedModels Time use " << (t1_1 - t1) << " ms";
#endif // DEBUG

	}

	void ModelSerialCommand::recoverModelFromSerialData(const QList<SAVED_INFO>& savedSerialInfos, QList<ModelN*>& newModels)
	{
		
		if (savedSerialInfos.isEmpty())
			return;

		newModels.clear();
		cxkernel::ModelNDataSerial dataSerializer;

		QList<cxkernel::ModelNDataPtr> serialdatas;

		for (const SAVED_INFO& savedInfo : savedSerialInfos)
		{
			dataSerializer.setData(nullptr);
			dataSerializer.load(savedInfo.serialName, nullptr);

			cxkernel::ModelNDataPtr data = dataSerializer.getData();
			if (!data)
			{
				qDebug() << QString("ModelNDataSerial error [%1]").arg(savedInfo.serialName);
			}
			serialdatas.append(data);
		}

		int count = serialdatas.count();
		for (int i = 0; i < count; i++)
		{
			cxkernel::ModelNDataPtr data = serialdatas.at(i);
			if (data)
			{
				data->input.name = savedSerialInfos.at(i).inputName;
				creative_kernel::ModelN* newModel = creative_kernel::createModelFromData(data);
				newModel->setSerialName(savedSerialInfos.at(i).serialName);
				newModel->setLocalData(savedSerialInfos.at(i).localPosition, savedSerialInfos.at(i).localQuaternion, savedSerialInfos.at(i).localScale);
				//newModel->setHoneyFillState(savedSerialInfos.at(i).isHoneyFilled);
				//newModel->setHoneyHollowed(savedSerialInfos.at(i).isHoneyHollowed);
				newModels.push_back(newModel);
			}
		}
	}
}