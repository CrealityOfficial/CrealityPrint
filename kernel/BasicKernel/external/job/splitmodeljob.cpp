#include "splitmodeljob.h"
#include "interface/spaceinterface.h"

#include <QtCore/QDebug>

#include "msbase/mesh/merge.h"

#include "interface/visualsceneinterface.h"
#include "qtuser3d/trimesh2/conv.h"

#include "qtusercore/module/progressortracer.h"
#include "interface/uiinterface.h"
#include <QCoreApplication>

#include "kernel/kernelui.h"
namespace creative_kernel
{
	SplitModelJob::SplitModelJob(QObject* parent)
		:Job(parent)
		, m_bSplit(false)
	{
	}

	SplitModelJob::~SplitModelJob()
	{
	}

	void SplitModelJob::EnableSplit(bool bSplit)
	{
		m_bSplit = bSplit;
	}

	QString SplitModelJob::name()
	{
		return "Generate Geometry";
	}

	QString SplitModelJob::description()
	{
		return "Generating Geometry.";
	}

	void SplitModelJob::failed()
	{
	}

	void SplitModelJob::successed(qtuser_core::Progressor* progressor)
	{
		if (m_modelGroups.size() <= 0)
			return;

		if (m_scene_create_data.add_groups.size() <= 0 && m_scene_create_data.remove_groups.size() <= 0)
			return;

		modifyScene(m_scene_create_data);
	}

	void SplitModelJob::work(qtuser_core::Progressor* progressor)
	{
		if (m_bSplit)
		{
			SplitModel(progressor);
		}
		else
		{
			MergeModel(progressor);
		}
	}

	void SplitModelJob::MergeModel(qtuser_core::Progressor* progressor)
	{
		if (m_modelGroups.size() <= 0)
			return;

		GroupCreateData group_create_data;  // create one merge group

		QList<GroupCreateData> group_remove_datas;  // remove the previous groups

		// set new modelGroup global transform
		qtuser_3d::Box3D gbox;
		for (ModelGroup* mg : m_modelGroups)
		{
			for (ModelN* m : mg->models())
			{
				gbox += m->globalSpaceBox();
			}
		}

		QVector3D groupBoxCenter = gbox.center();
		groupBoxCenter.setZ(0.0f);
		trimesh::vec3 offset = qtuser_3d::qVector3D2Vec3(groupBoxCenter);
		group_create_data.xf = trimesh::fxform::trans(offset);

		for (ModelGroup* mg : m_modelGroups)
		{
			GroupCreateData one_remove_group_data;
			one_remove_group_data.reset();

			for(int i = 0; i < mg->models().size(); i++)
			{
				ModelN* model = mg->models().at(i);
				if (model)
				{
					ModelCreateData model_create_data;
					model_create_data.model_id = -1;
					model_create_data.fixed_id = model->getFixedId();
					// model_create_data.data = model->modelNData();
					model_create_data.dataID = model->sharedDataID();

					trimesh::vec3 offset = qtuser_3d::qVector3D2Vec3(groupBoxCenter);
					QVector3D componentOffset = model->globalSpaceBox().center() - groupBoxCenter;
					model_create_data.xf = trimesh::fxform::trans(qtuser_3d::qVector3D2Vec3(componentOffset));

					group_create_data.models.append(model_create_data);

					ModelCreateData model_remove_data;
					model_remove_data.model_id = model->getObjectId();
					model_remove_data.fixed_id = model->getFixedId();
					// model_remove_data.data = model->modelNData();
					model_remove_data.dataID = model->sharedDataID();
					model_remove_data.xf = model->getMatrix();

					one_remove_group_data.model_group_id = mg->getObjectId();
					one_remove_group_data.name = mg->groupName();
					one_remove_group_data.xf = mg->getMatrix();
					one_remove_group_data.models.append(model_remove_data);

				}

			}

			group_remove_datas.append(one_remove_group_data);
			
		}

		m_scene_create_data.add_groups.append(group_create_data);
		m_scene_create_data.remove_groups.append(group_remove_datas);

	}

	void SplitModelJob::SplitModel(qtuser_core::Progressor* progressor)
	{
		if (m_modelGroups.size() <= 0 || m_modelGroups.size() > 1)
			return;

		GroupCreateData group_remove_data;  // remove the selected modelGroup
		group_remove_data.name = m_modelGroups[0]->groupName();
		group_remove_data.model_group_id = m_modelGroups[0]->getObjectId();
		group_remove_data.xf = m_modelGroups[0]->getMatrix();

		QList<GroupCreateData> group_create_datas;  // add some modelGroups according to models in the selected modelGroup

		for (ModelGroup* mg : m_modelGroups)
		{
			for (int i = 0; i < mg->models().size(); i++)
			{
				ModelN* model = mg->models().at(i);
				if (model)
				{
					GroupCreateData one_group_data;
					one_group_data.reset();

					ModelCreateData model_remove_data;
					model_remove_data.model_id = model->getObjectId();
					model_remove_data.fixed_id = model->getFixedId();
					model_remove_data.dataID = model->sharedDataID();
					model_remove_data.xf = model->getMatrix();
					group_remove_data.models.append(model_remove_data);

					ModelCreateData model_create_data;
					model_create_data.model_id = -1;
					model_create_data.fixed_id = model->getFixedId();

					float zPos = model->localPosition().z();
					model_create_data.xf = trimesh::xform::trans(QVector3D(0.0f, 0.0f, zPos));

					model_create_data.dataID = model->sharedDataID();

					one_group_data.models.append(model_create_data);
					QVector3D gPos = model->globalSpaceBox().center();
					gPos.setZ(0.0f);
					one_group_data.xf = trimesh::xform::trans(gPos);

					group_create_datas.append(one_group_data);

				}

			}

		}

		m_scene_create_data.add_groups.append(group_create_datas);
		m_scene_create_data.remove_groups.append(group_remove_data);
	}

	void SplitModelJob::setModelGroups(const QList<ModelGroup*> modelGroups)
	{
		m_modelGroups = modelGroups;
	}

}
