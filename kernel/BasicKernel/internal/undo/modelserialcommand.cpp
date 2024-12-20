#include "modelserialcommand.h"
#include "kernel/kernel.h"
#include "data/modelspace.h"
#include "kernel/visualscene.h"
#include "msbase/data/conv.h"
#include "qtuser3d/trimesh2/conv.h"

#include "interface/spaceinterface.h"
#include "interface/selectorinterface.h"

#include "internal/render/modelnentity.h"
#include "internal/render/modelgroupentity.h"
#include "interface/projectinterface.h"
#include "internal/project_cx3d/autosavejob.h"
namespace creative_kernel
{
	///////////////
	SpaceSerialCommand::SpaceSerialCommand(const SceneCreateData& data, QObject* parent)
		:QObject(parent)
	{
		//check
		ModelSpace* space = getModelSpace();
		for (const GroupCreateData& group_data : data.remove_groups)
		{
			ModelBunch bunch;
			ModelGroup* group = space->getModelGroupByObjectId(group_data.model_group_id);
			assert(group);
			bunch._model_group = group;

			if (group_data.models.size() == 0) {
				bunch.models = group->models();
			}else {
				for (const ModelCreateData& model_data : group_data.models)
				{
					ModelN* _model = space->getModelNByObjectId(model_data.model_id);
					assert(_model);
					bunch.models.append(_model);
				}

			}
			m_data.remove_groups.append(bunch);
		}

		for (const GroupCreateData& group_data : data.add_groups)
		{
			ModelBunch bunch;
			ModelGroup* _model_group = space->getModelGroupByObjectId(group_data.model_group_id);
			if (!_model_group)
			{
				assert(msbase::checkValid(group_data.xf));
				assert(group_data.models.size() > 0);
				_model_group = space->createModelGroup(this, group_data.model_group_id);
				_model_group->setGroupName(group_data.name);
				_model_group->setting()->merge(group_data.usettings);

				_model_group->setDefaultColorIndex(group_data.defaultColorIndex);
				_model_group->setMatrix(group_data.xf);
				if(group_data.model_group_id == -1)
					_model_group->setInitMatrix();
				QString name = group_data.name;
				if (name.isEmpty())
					name = "Group";
				_model_group->setGroupName(name);

				if (group_data.outlinePath.size() > 0)
				{
					_model_group->copyOutlinePath(group_data.outlinePath);  // cache copy group outlinePath data
					_model_group->setOutlinePathInitPos(group_data.outlinePathInitPos);
				}

				_model_group->setLayerHeightProfile(group_data.layerHeightProfile);
					
			}

			for (const ModelCreateData& model_data : group_data.models)
			{
				assert(model_data.model_id == -1);
				QString name = model_data.name;
				if (name.isEmpty())
					name = "Part";
				assert(msbase::checkValid(model_data.xf));

				ModelN* _model = space->createModel(this, model_data.fixed_id, model_data.model_id);
				_model->setSharedData(model_data.dataID);
				_model->setName(name);
				_model->setting()->merge(model_data.usettings);
				_model->setFontMesh(model_data.reliefDescription);
				_model->setMatrix(model_data.xf);
				_model->setInitMatrix();
				_model->setLayerHeightProfile(model_data.layerHeightProfile);
				_model->setPartIndex(model_data.model_part_index);

				bunch.models.append(_model);
			}

			bunch._model_group = _model_group;
			m_data.add_groups.append(bunch);
		}
	}

	SpaceSerialCommand::~SpaceSerialCommand()
	{

	}

	void SpaceSerialCommand::undo()
	{
		invoke(m_data.remove_groups, m_data.add_groups);
	}

	void SpaceSerialCommand::redo()
	{
		invoke(m_data.add_groups, m_data.remove_groups);
	}

	void SpaceSerialCommand::invoke(const QList<ModelBunch>& add_bunchs, const QList<ModelBunch>& remove_bunchs)
	{
		QList<ModelGroup*> add_model_groups;
		for (const ModelBunch& bunch : add_bunchs)
		{
			add_model_groups << bunch._model_group;
		}
		if(add_model_groups.size()>0)
			triggerAutoSave(add_model_groups,AutoSaveJob::MESH);
			

		QList<ModelGroup*> real_remove_model_groups;
		for (const ModelBunch& bunch : remove_bunchs)
		{
			if (add_model_groups.contains(bunch._model_group))
				continue;

			if (bunch.models.size() < bunch._model_group->models().size())
				continue;

			real_remove_model_groups.append(bunch._model_group);
		}
		if(real_remove_model_groups.size()>0)
			triggerAutoSave(QList<creative_kernel::ModelGroup*>(),AutoSaveJob::MESH);

		ModelSpace* space = getModelSpace();

		QList<ModelGroup*> real_add_model_groups;
		for (ModelGroup* model_group : add_model_groups)
		{
			if (!space->getModelGroupByObjectId(model_group->getObjectId()))
				real_add_model_groups.append(model_group);
		}

		invokeModify(real_add_model_groups, real_remove_model_groups, add_bunchs, remove_bunchs);
	}

	void SpaceSerialCommand::invokeModify(const QList<ModelGroup*>& add_model_groups, const QList<ModelGroup*>& remove_model_groups,
		const QList<ModelBunch>& add_models, const QList<ModelBunch>& remove_models)
	{
		ModelSpace* space = getModelSpace();
		VisualScene* scene = getKernel()->visScene();

		//remove render entity
		for (ModelGroup* _model_group : remove_model_groups)
		{
			scene->destroyModelGroupEntity(_model_group->entity());
			_model_group->setEntity(nullptr);
			_model_group->resetOutlineState();
		}
		for (const ModelBunch& bunch : remove_models)
		{
			for (ModelN* _model : bunch.models)
			{
				scene->destroyModelNEntity(_model->entity());
				_model->setEntity(nullptr);
			}
		}

		//modify space
		space->addModelGroups_ur(add_model_groups);
		for (const ModelBunch& bunch : add_models)
		{
			space->addModels_ur(bunch.models, bunch._model_group);
		}
		for (const ModelBunch& bunch : remove_models)
		{
			space->removeModels_ur(bunch.models, bunch._model_group);
			for (ModelN* _model : bunch.models)
				_model->setParent(this);
		}
		space->removeModelGroups_ur(remove_model_groups);
		for (ModelGroup* _model_group : remove_model_groups)
			_model_group->setParent(this);

		//add render entity
		for (ModelGroup* _model_group : add_model_groups)
		{
			ModelGroupEntity* entity = scene->createModelGroupEntity();
			scene->show(entity, VisualScene::ModelSpace);
			_model_group->setEntity(entity);
		}

		space->updateSpaceNodeRender(add_model_groups);
		for (const ModelBunch& bunch : add_models)
		{
			for (ModelN* _model : bunch.models)
			{
				ModelNEntity* entity = scene->createModelNEntity();
				entity->setParent(bunch._model_group->entity());
				entity->setEnabled(true);
				_model->setEntity(entity);

				// fix bug : if the model has been sliced before, and then delete the model from scene, 
				//           and then undo the deleted operation, slice the recovered model will crash
				_model->needRegenerate();
			}

			space->updateSpaceNodeRender(bunch.models);
		}
		//notify
		space->notifySpaceChange(true);
		selectGroups(add_model_groups);
	}

}