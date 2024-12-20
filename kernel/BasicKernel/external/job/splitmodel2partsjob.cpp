#include "splitmodel2partsjob.h"

#include "interface/spaceinterface.h"

#include <QtCore/QDebug>

#include "interface/visualsceneinterface.h"
#include "interface/shareddatainterface.h"
#include "qtuser3d/trimesh2/conv.h"

#include "qtusercore/module/progressortracer.h"
#include "interface/uiinterface.h"
#include <QCoreApplication>

#include "kernel/kernelui.h"
#include "msbase/mesh/merge.h"

namespace creative_kernel
{
	SplitModel2PartsJob::SplitModel2PartsJob(QObject* parent)
		:Job(parent)
		, m_model(nullptr)
		, m_split2ObjectFlag(false)
	{
	}

	SplitModel2PartsJob::~SplitModel2PartsJob()
	{
	}

	QString SplitModel2PartsJob::name()
	{
		return "Generate Geometry";
	}

	QString SplitModel2PartsJob::description()
	{
		return "Generating Geometry.";
	}

	void SplitModel2PartsJob::failed()
	{
	}

	void SplitModel2PartsJob::successed(qtuser_core::Progressor* progressor)
	{
		if (m_datas.size() <= 1)
		{
			if (m_split2ObjectFlag)
				getKernelUI()->requestQmlTipDialog(QCoreApplication::translate("AllMenuDialog", "The currently selected model cannot be split."));
			else
				getKernelUI()->requestQmlTipDialog(QCoreApplication::translate("AllMenuDialog", "The object only contains one part and cannot be split."));
			return;
		}

		if (!m_model)
			return;

		if (m_split2ObjectFlag)
		{
			createObjects();
		}
		else
		{
			createParts();
		}

	}

	void SplitModel2PartsJob::work(qtuser_core::Progressor* progressor)
	{
		SplitModel(progressor);
	}

	void SplitModel2PartsJob::SplitModel(qtuser_core::Progressor* progressor)
	{
		if (!m_model || !m_model->getModelGroup())
			return;

		std::vector<TriMeshPtr> meshes;
		meshes.push_back(TriMeshPtr(m_model->createGlobalMesh()));

		qtuser_core::ProgressorTracer tracer(progressor);
		std::vector<std::vector<TriMeshPtr>> out = msbase::meshSplit(meshes, &tracer);

		if (progressor)
		{
			progressor->progress(1);
		}

		qtuser_3d::Box3D gbox = m_model->getModelGroup()->globalBox();
		QVector3D groupBoxCenter = gbox.center();

		QString name = m_model->objectName();
		const std::vector<TriMeshPtr>& outMeshes = out.at(0);
		for (int id = 0; id < outMeshes.size(); ++id)
		{
			QString subName = QString("%1-split-parts-%2").arg(name).arg(id);

			cxkernel::ModelCreateInput input;
			cxkernel::ModelNDataCreateParam param;
			input.mesh = outMeshes.at(id);
			input.name = subName;

			// optimize to do !
			qtuser_3d::Box3D compbox;
			for (const trimesh::vec3& v : input.mesh->vertices)
			{
				compbox += qtuser_3d::vec2qvector(v);
			}

			if (compbox.size().x() * compbox.size().y() * compbox.size().z() < 1.0f)
				continue;

			m_split2ObjectPoses.append(compbox.center());

			QVector3D compOffset = compbox.center() - groupBoxCenter;

			ModelDataPtr model_data = createModelData(input.mesh, input.colors, input.seams, input.supports, input.defaultColor);
			if (!model_data)
				continue;

			m_datas.append(model_data);
			m_componentOffsets.append(compOffset);
		}
	}

	// split to modeln parts
	void SplitModel2PartsJob::createParts()
	{
		ModelGroup* modelGroup = m_model->getModelGroup();
		if (!modelGroup)
			return;

		creative_kernel::SceneCreateData scene_create_data;

		GroupCreateData group_remove_data;  // remove the selected modelGroup
		group_remove_data.name = modelGroup->groupName();
		group_remove_data.model_group_id = modelGroup->getObjectId();

		GroupCreateData group_create_data;

		QString groupName = modelGroup->groupName();

		qtuser_3d::Box3D gbox = modelGroup->globalBox();

		QVector3D groupBoxCenter = gbox.center();
		trimesh::vec3 gbCenter = qtuser_3d::qVector3D2Vec3(groupBoxCenter);

		group_create_data.xf = trimesh::fxform::trans(gbCenter);
		group_create_data.name = groupName;

		for (int i = 0; i < m_datas.size(); i++)
		{
			QString tmpName = groupName + "_" + QString::number(i + 1);
			ModelDataPtr data = m_datas.at(i);

			ModelCreateData model_create_data;

			model_create_data.dataID = registerModelData(data);
			model_create_data.name = tmpName;

			model_create_data.xf = trimesh::xform::trans(m_componentOffsets.at(i));
			model_create_data.usettings = modelGroup->setting()->copy();

			group_create_data.models.append(model_create_data);

		}

		scene_create_data.add_groups.append(group_create_data);
		scene_create_data.remove_groups.append(group_remove_data);

		modifyScene(scene_create_data);
	}

	// split to group objects
	void SplitModel2PartsJob::createObjects()
	{
		ModelGroup* modelGroup = m_model->getModelGroup();
		if (!modelGroup)
			return;

		creative_kernel::SceneCreateData scene_create_data;

		GroupCreateData group_remove_data;  // remove the selected modelGroup
		group_remove_data.name = modelGroup->groupName();
		group_remove_data.model_group_id = modelGroup->getObjectId();

		QString groupName = modelGroup->groupName();

		GroupCreateData group_create_data;
		for (int i = 0; i < m_datas.size(); i++)
		{
			group_create_data.reset();

			QString tmpName = groupName + "_" + QString::number(i + 1);

			ModelDataPtr data = m_datas.at(i);

			QVector3D objPos = m_split2ObjectPoses.at(i);
			trimesh::dbox3 b = data->mesh->calculateBox();
			group_create_data.xf = trimesh::fxform::trans(objPos.x(), objPos.y(), -b.min.z);
			group_create_data.usettings = modelGroup->setting()->copy();
			group_create_data.name = tmpName;

			ModelCreateData model_create_data;

			model_create_data.dataID = registerModelData(data);
			model_create_data.name = tmpName;

			model_create_data.xf = trimesh::xform::trans(0.0f, 0.0f, 0.0f);
			model_create_data.usettings = modelGroup->setting()->copy();

			group_create_data.models.append(model_create_data);

			scene_create_data.add_groups.append(group_create_data);

		}

		scene_create_data.remove_groups.append(group_remove_data);

		modifyScene(scene_create_data);
	}

	void SplitModel2PartsJob::setModel(ModelN* model)
	{
		m_model = model;
	}

	void SplitModel2PartsJob::setSplit2ObjectFlag(bool bFlag)
	{
		m_split2ObjectFlag = bFlag;
	}

}
