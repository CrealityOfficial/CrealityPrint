#include "modelspace.h"
#include "data/modeln.h"
#include "kernel/modelnselector.h"
#include "cxkernel/utils/utils.h"

#include "qtuser3d/trimesh2/conv.h"

#include "interface/reuseableinterface.h"
#include "interface/renderinterface.h"
#include "interface/visualsceneinterface.h"
#include "interface/parameterinterface.h"
#include "interface/uiinterface.h"

#include "interface/printerinterface.h"
#include "internal/multi_printer/printer.h"
#include "interface/shareddatainterface.h"

#include "message/supportgeneratormessage.h"

namespace creative_kernel
{
	ModelCreateData generateModelCreateData(ModelN* model)
	{
		assert(model);
		ModelCreateData data;
		data.model_id = model->getObjectId();
		data.dataID = model->sharedDataID();
		data.name = model->name();
		data.xf = trimesh::xform(qtuser_3d::qMatrix2Xform(model->localMatrix()));
		data.usettings = model->setting()->copy();
		data.reliefDescription = model->fontMeshString();
		return data;
	}

	GroupCreateData generateGroupCreateData(ModelGroup* _model_group, bool generateModel)
	{
		assert(_model_group);
		GroupCreateData data;
		data.model_group_id = _model_group->getObjectId();
		data.name = _model_group->groupName();
		data.xf = trimesh::xform(qtuser_3d::qMatrix2Xform(_model_group->localMatrix()));
		data.usettings = _model_group->setting()->copy();
		if (generateModel)
		{
			QList<ModelN*> models = _model_group->models();
			for (ModelN* model : models)
			{
				data.models.append(generateModelCreateData(model));
			}
		}
		return data;
	}

	int64_t ModelSpace::generateUniqueId()
	{
		return m_uniqueID++;
	}
	
	int64_t ModelSpace::generateFixedId()
	{
		return m_uniqueFixedID++;
	}

	void ModelSpace::onModelNTypeChanged(QObject* obj)
	{
		ModelN* model = dynamic_cast<ModelN*>(obj);
		if (!model)
			return;

		for (SpaceTracer* tracer : m_spaceTracers)
		{
			tracer->onModelTypeChanged(model);
		}
		onModelTypeChanged(model);
	}

	void ModelSpace::notifySpaceChange(bool modelChanged)
	{
		if (modelChanged)
		{
			m_model_selector->updateFaceBases();
			emit modelNNumChanged();
			requestVisPickUpdate(true);
		}

		trimesh::dbox3 box = sceneBoundingBox();
		// Do not use for auto here, get the cend() iterator once in each loop to ensure its validity.
		for (auto iter = m_spaceTracers.cbegin(); iter != m_spaceTracers.cend(); ++iter) {
			(*iter)->onSceneChanged(box);
		}

		printSpace();
	}

	void ModelSpace::checkModelGroupSupportGenerator(ModelGroup* group)
	{
		if (group)
		{
			QString enabled = group->setting()->value("enable_support", "");
			if (enabled.isEmpty())
				enabled = getPrintProfileValue("enable_support", "");

			if (enabled == "0")
			{
				QList<ModelN*> part = group->models((int)ModelNType::support_generator);
				if (!part.isEmpty())
				{
					SupportGeneratorMessage* message = new SupportGeneratorMessage(part.first());
					requestQmlMessage(message);
				}
			}
		}
		else
		{
			Printer* printer = getSelectedPrinter();
			ModelN* supportGenerator = NULL;
			for (ModelGroup* group : printer->modelGroupsInside())
			{
				QString enabled = group->setting()->value("enable_support", "");
				if (enabled.isEmpty())
					enabled = getPrintProfileValue("enable_support", "");

				if (enabled == "0")
				{
					QList<ModelN*> part = group->models((int)ModelNType::support_generator);
					if (!part.isEmpty())
						supportGenerator = part.first();
				}

				if (supportGenerator)
					break;
			}

			if (supportGenerator)
			{
				SupportGeneratorMessage* message = new SupportGeneratorMessage(supportGenerator);
				requestQmlMessage(message);
			}
			else
			{
				destroyQmlMessage(SupportGeneratorWarn);
			}
		}
	}

	void ModelSpace::update_crslice2_matrix(const trimesh::xform& offset_xf)
	{
		for (ModelGroup* _group : m_sceneModelGroups)
		{
			crslice2::CrSliceObject* object = _group->m_crslice_object;
			if (object)
			{
				object->setMatrix(offset_xf * _group->getMatrix());
			}

			for (ModelN* model : _group->models())
			{
				SharedDataID id = model->sharedDataID();
				crslice2::CrSliceVolume* volume = model->m_crslice_volume;
				if (volume)
				{
					volume->setMatrix(model->getMatrix());
				}
			}
		}
	}

	void ModelSpace::update_crslice2_parameter()
	{
		for (ModelGroup* _group : m_sceneModelGroups)
		{
			crslice2::CrSliceObject* object = _group->m_crslice_object;
			if (object)
			{
				object->setName(cxkernel::qString2String(_group->groupName()));
				object->setHostID(_group->getObjectId());
				object->setLayerHeight(_group->m_layerHeightProfile);
				object->setVisible(_group->isVisible());
				object->clearParameter();
				const QHash<QString, us::USetting*>& MG = _group->setting()->settings();
				for (QHash<QString, us::USetting*>::const_iterator it = MG.constBegin(); it != MG.constEnd(); ++it)
				{
					object->setParameter(it.key().toStdString(), it.value()->str().toStdString());
				}
			}

			for (ModelN* model : _group->models())
			{
				SharedDataID id = model->sharedDataID();
				crslice2::CrSliceVolume* volume = model->m_crslice_volume;
				if (volume)
				{
					volume->setName(cxkernel::qString2String(model->name()));
					volume->setModelType((int)model->modelNType());
					volume->setHostID(model->getObjectId());

					if (model->m_need_regenerate_id)
					{
						volume->regenerateID();
						volume->setMeshData(getMeshData(id)->mesh);
						volume->setSpreadColor(getColors(id));
						volume->setSpreadSeam(getSeams(id));
						volume->setSpreadSupport(getSupports(id));
						model->m_need_regenerate_id = false;
					}
					volume->clearParameter();
					const QHash<QString, us::USetting*>& MS = model->setting()->settings();
					for (QHash<QString, us::USetting*>::const_iterator it = MS.constBegin(); it != MS.constEnd(); ++it)
					{
						volume->setParameter(it.key().toStdString(), it.value()->str().toStdString());
					}
				}
			}
		}
	}

	void ModelSpace::onModelGroupModified(ModelGroup* _model_group, const QList<ModelN*>& removes, const QList<ModelN*>& adds)
	{
		checkModelGroupSupportGenerator();
	}

	void ModelSpace::onModelTypeChanged(ModelN* model)
	{
		checkModelGroupSupportGenerator();
	}

	void ModelSpace::onSupportEnabledChanged()
	{
		checkModelGroupSupportGenerator();
	}

	void ModelSpace::onSelectedPrinterChanged(Printer* printer)
	{
		if (m_selectedPrinter)
		{
			disconnect(m_selectedPrinter, &Printer::signalModelsInsideChange, this, &ModelSpace::onSelectedPrinterModelsChanged);
		}

		m_selectedPrinter = printer;

		if (m_selectedPrinter)
		{
			connect(m_selectedPrinter, &Printer::signalModelsInsideChange, this, &ModelSpace::onSelectedPrinterModelsChanged);
		}
		checkModelGroupSupportGenerator();
	}

	void ModelSpace::onSelectedPrinterModelsChanged()
	{
		checkModelGroupSupportGenerator();
	}

	void ModelSpace::updateSpaceNodeRender(ModelGroup* group, bool capture)
	{
		if (group)
		{
			QList<ModelGroup*> groups;
			groups << group;
			updateSpaceNodeRender(groups, capture);
		} 
	}

	void ModelSpace::updateSpaceNodeRender(const QList<ModelGroup*>& groups, bool capture)
	{
		if (groups.size() > 0)
		{
			for (ModelGroup* model_group : groups)
			{
				model_group->updateNode();
				model_group->dirty();
			}

			m_model_selector->changeModelGroups(groups);
			requestVisPickUpdate(capture);
		}
	}

	void ModelSpace::updateSpaceNodeRender(ModelN* model, bool capture)
	{
		if (model)
		{
			QList<ModelN*> models;
			models << model;
			updateSpaceNodeRender(models, capture);
		}
	}

	void ModelSpace::updateSpaceNodeRender(const QList<ModelN*>& models, bool capture)
	{
		if (models.size() > 0)
		{
			for (ModelN* model : models)
			{
				model->updateNode();
				model->dirty();
			}

			m_model_selector->changeModels(models);
			requestVisPickUpdate(capture);
		}
	}

	void ModelSpace::printSpace()
	{
#if _DEBUG
		qDebug() << "1111111111111111";
		for (ModelGroup* group : m_sceneModelGroups)
		{
			qDebug() << QString("model group : %1").arg(group->getObjectId());
			for (ModelN* model : group->models())
			{
				qDebug() << QString("*** model : %1").arg(model->getObjectId());
			}
		}
		qDebug() << "tracer count:" << m_model_selector->modelTracersCount();
#endif
	}

}
