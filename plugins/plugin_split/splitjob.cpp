#include "splitjob.h"

#include "CUT/cut.h"

#include "trimesh2/TriMesh_algo.h"
#include "trimesh2/XForm.h"
#include <stack>
#include "qtuser3d/math/const.h"
#include "qtuser3d/trimesh2/conv.h"
#include "msbase/mesh/tinymodify.h"

#include "interface/shareddatainterface.h"
#include "interface/selectorinterface.h"
#include "interface/spaceinterface.h"
#include "data/spaceutils.h"

using namespace creative_kernel;
SplitJob::SplitJob(QObject* parent)
	: Job(parent)
{
}

SplitJob::~SplitJob()
{
}

bool SplitJob::isTriMeshAtPlaneForward(trimesh::TriMesh* mesh, trimesh::vec3 pos, trimesh::vec3 dir)
{
	bool atPlaneForward = true;
	for (trimesh::vec3 v : mesh->vertices)
	{
		float d = (v - pos).dot(dir);
		if (d > 0.01)
		{
			atPlaneForward = true;
			break;
		}
		else if (d < -0.01)
		{
			atPlaneForward = false;
			break;
		}
	}
	return atPlaneForward;
}

QVector3D SplitJob::getOffset(bool isForward, trimesh::vec3 direction, float gap)
{
	trimesh::vec3 offsetDir = isForward ? direction : -direction;
	trimesh::vec3 offset;

	if (offsetDir == trimesh::vec3(0.0f, 0.0f, 1.0f)) 
	{
		offset = gap * offsetDir;
	}
	else if (offsetDir == trimesh::vec3(0.0f, 0.0f, -1.0f)) 
	{	// 贴底一侧不需要平移
		//center.z;
	}
	else 
	{	// 其它方向水平平移
		offsetDir[2] = 0;
		trimesh::normalize(offsetDir);
		double offsetLen = gap / 2.0;
		offset = offsetDir * offsetLen;
	}

	return QVector3D(offset.x, offset.y, offset.z);
}

QString SplitJob::name()
{
	return "Split";
}

QString SplitJob::description()
{
	return "";
}

void SplitJob::setParts(const QList<creative_kernel::ModelN*>& models, creative_kernel::ModelGroup* modelGroup)
{
	m_models = models;
	m_modelGroup = modelGroup;
}

void SplitJob::setModelGroups(const QList<creative_kernel::ModelGroup*>& modelgroup)
{
	m_modelGroups = modelgroup;
}

void SplitJob::setCutParam(const CutParam& param)
{
	m_param = param;
}

void SplitJob::prepareWork()
{
	if (m_modelGroup)
	{
		m_modelGroups.clear();
		m_modelGroups << m_modelGroup;
	}

	for (ModelGroup* modelGroup : m_modelGroups)
	{
		trimesh::xform groupMatrix = trimesh::xform(modelGroup->globalMatrix().data());
		modelGroup->setMatrix(trimesh::xform::identity());
		for (ModelN* model : modelGroup->models())
		{
			model->setMatrix(groupMatrix * model->getMatrix());
		}
	}
}

void SplitJob::work(qtuser_core::Progressor* progressor)
{
	CUT::CutPlane aplane;
	aplane.normal = trimesh::vec3(m_param.normal);
	aplane.position = trimesh::vec3(m_param.position);

	progressor->progress(0.1f);

	for (ModelGroup* modelGroup : m_modelGroups)
	{
		QList<ModelN*> models;
		if (m_modelGroup)
			models = m_models;
		else
			models = modelGroup->models((int)ModelNType::normal_part);

		QList<ModelN*> groupModels;
		if (!m_param.isCutGroup)
			groupModels = models;

		for (ModelN* model : models)
		{
			if (!m_param.isCutGroup)
				groupModels.removeOne(model);

			TriMeshPtr input(model->createGlobalMeshWithNormal());

			if (!input)
				continue;

			std::vector<trimesh::TriMesh*> result_meshes;
			CUT::planeCut(input.get(), aplane, result_meshes);

			//trimesh::xform globalXf = model->getMatrix() * modelGroup->getMatrix();
			//trimesh::xform globalXf = model->getMatrix() * modelGroup->getMatrix();
			trimesh::xform globalXf(model->globalMatrix().data());
			invert(globalXf);

			for (int i = 0, count = result_meshes.size(); i < count; ++i)
			{
				TriMeshPtr invertedMesh(new trimesh::TriMesh());
				*invertedMesh = *(result_meshes[i]);

				trimesh::apply_xform(invertedMesh.get(), globalXf);

				trimesh::fxform normalXf = qtuser_3d::qMatrix2Xform(model->modelNormalMatrix());

				creative_kernel::changeMeshFaceNormal(invertedMesh.get(), result_meshes[i], normalXf);

				trimesh::apply_xform(result_meshes[i], globalXf);
			}

			if (result_meshes.size() == 2)
			{
				SplitResult result;
				result.model = model;
				{
					bool isForward = isTriMeshAtPlaneForward(result_meshes.at(0), globalXf * aplane.position, aplane.normal);
					cxkernel::MeshDataPtr meshData = createMeshDataPtr(TriMeshPtr(result_meshes.at(0)), true);
					int id = registerMeshData(meshData);
					meshData->mesh->bbox.max;
					result.directions << isForward;
					if (!m_param.cut2Parts)
						result.splitIDs << isForward;
					else
						result.splitIDs << 0;
					result.meshIDs << id;
					trimesh::xform tempXf(model->globalMatrix().data());;
					tempXf[12] = tempXf[13] = tempXf[14] = 0;
					trimesh::dvec3 center = -(tempXf * meshData->offset);
					result.centers << QVector3D(center.x, center.y, center.z);
				}

				{
					bool isForward = isTriMeshAtPlaneForward(result_meshes.at(1), globalXf * aplane.position, aplane.normal);
					cxkernel::MeshDataPtr meshData = createMeshDataPtr(TriMeshPtr(result_meshes.at(1)), true);
					int id = registerMeshData(meshData);
					result.directions << isForward;
					if (!m_param.cut2Parts)
						result.splitIDs << isForward;
					else
						result.splitIDs << 0;
					result.meshIDs << id;
					trimesh::xform tempXf(model->globalMatrix().data());;
					tempXf[12] = tempXf[13] = tempXf[14] = 0;
					trimesh::dvec3 center = -(tempXf * meshData->offset);
					result.centers << QVector3D(center.x, center.y, center.z);
				}

				m_results[modelGroup].append(result);
			}
			else
			{
				//qDeleteAll(result_meshes);
				groupModels << model;
			}
		}

		for (ModelN* model : groupModels)
		{
			SplitResult result;
			// result.modelGroup = modelGroup;
			result.model = model;
			result.meshIDs << model->meshID();

			trimesh::xform globalXf = model->getMatrix() * modelGroup->getMatrix();
			invert(globalXf);
			trimesh::xform xf = model->getMatrix();
			invert(xf);
			bool isForward = isTriMeshAtPlaneForward(model->data()->mesh.get(), globalXf * aplane.position, aplane.normal);
			result.directions << isForward;

			QVector3D center = model->globalSpaceBox().center();
			trimesh::vec3 tcenter(center.x(), center.y(), center.z());
			tcenter = globalXf * tcenter;
			result.centers << QVector3D(tcenter.x, tcenter.y, tcenter.z);

			if (!m_param.cut2Parts)
			{
				result.splitIDs << isForward;
			}
			else
				result.splitIDs << 0;

			m_results[modelGroup].append(result);
			// m_results.append(result);
		}
	}

	if (m_results.size() == 0)
		progressor->failed("");
	progressor->progress(1.0f);
}

void SplitJob::failed()
{

}

void SplitJob::successed(qtuser_core::Progressor* progressor)
{
	if (m_results.size() <= 0)
		return;

	//切割输入只能接受group类型.
	//切割为零件标志：
	//如果为true,界面打上勾,表示：继续保持一个group; 模型被切割成两个A,B; 模型的位置姿态保持原样不变。
	//如果为false,界面不打勾,表示：则生产两个新group; 模型被切割成两个；每个group里面放一个。

	SceneCreateData  scene_data;
	auto it = m_results.begin(), end = m_results.end();
	if (m_param.cut2Parts)
	{
		for (; it != end; ++it)
		{
			ModelGroup* modelGroup = it.key();
			const QList<SplitResult>& results = it.value();

			GroupCreateData remove = generateGroupCreateData(modelGroup, false);
			GroupCreateData add = generateGroupCreateData(modelGroup, false);
			for (const SplitResult& result : results)
			{
				assert(result.model->getModelGroup() == modelGroup);
				ModelCreateData model_create_data;
				model_create_data.model_id = result.model->getObjectId();
				remove.models.append(model_create_data);

				bool isSplit = result.meshIDs.size() > 1;
				for (int i = 0; i < result.meshIDs.size(); ++i)
				{
					ModelCreateData create_data;
					if (isSplit)
						create_data.name = QString("%1#%2").arg(result.model->name()).arg(i);
					else 
						create_data.name = result.model->name();

					QVector3D c = result.centers[i];
					//if (result.model->isRelief() && !isSplit)
					//	c = QVector3D(0, 0, 0);

					QVector3D offset = getOffset(result.directions[i], m_param.normal, m_param.gap / 2.0);
					trimesh::xform xf = result.model->getMatrix();
					c += offset;
					xf[12] += c.x();
					xf[13] += c.y();
					xf[14] += c.z();
					create_data.xf = xf;

					SharedDataID id;
					id(MeshID) = result.meshIDs.at(i);
					id(MaterialID) = result.model->sharedDataID()(MaterialID);
					id(ModelType) = (int)result.model->modelNType();

					if (!isSplit)
					{	/* reserve attribute */
						id(ColorsID) = result.model->colorsID();
						id(SeamsID) = result.model->seamsID();
						id(SupportsID) = result.model->supportsID();
						create_data.reliefDescription = result.model->fontMeshString();
					}

					create_data.dataID = id;
					add.models.append(create_data);
				}
			}

			scene_data.add_groups.append(add);
			scene_data.remove_groups.append(remove);
		}
	}
	else
	{
		for (; it != end; ++it)
		{
			ModelGroup* modelGroup = it.key();
			const QList<SplitResult>& results = it.value();

			GroupCreateData remove = generateGroupCreateData(modelGroup, true);
			scene_data.remove_groups.append(remove);

			GroupCreateData add1, add2;
			bool add1Enable = false, add2Enable = false;
			add1.xf = modelGroup->getMatrix();
			add1.name = modelGroup->groupName();
			add2.xf = modelGroup->getMatrix();
			add2.name = modelGroup->groupName();


			for (const SplitResult& result : results)  
			{ // to do , not offset
				assert(result.model->getModelGroup() == modelGroup);
				bool isSplit = result.meshIDs.size() > 1;
				for (int i = 0; i < result.meshIDs.size(); ++i)
				{
					ModelCreateData create_data;
					create_data.name = QString("%1#%2").arg(result.model->name()).arg(i);
					//create_data.xf = result.model->getMatrix();

					QVector3D c = result.centers[i];
					QVector3D offset = getOffset(result.directions[i], m_param.normal, m_param.gap / 2.0);
					trimesh::xform xf = result.model->getMatrix();
					c += offset;
					xf[12] += c.x();
					xf[13] += c.y();
					xf[14] += c.z();
					create_data.xf = xf;

					SharedDataID id;
					id(MeshID) = result.meshIDs.at(i);
					id(MaterialID) = result.model->sharedDataID()(MaterialID);
					id(ModelType) = (int)result.model->modelNType();
					if (!isSplit)
					{	/* reserve attribute */
						id(ColorsID) = result.model->colorsID();
						id(SeamsID) = result.model->seamsID();
						id(SupportsID) = result.model->supportsID();
						create_data.reliefDescription = result.model->fontMeshString();
					}
					create_data.dataID = id;

					if (result.splitIDs[i] == 0)
					{
						add1.models.append(create_data);
						add1Enable = true;
					}
					else 
					{
						add2.models.append(create_data);
						add2Enable = true;
					}
				}
			}

			if (add1Enable)
				scene_data.add_groups.append(add1);

			if (add2Enable)
				scene_data.add_groups.append(add2);
		}
	}

	creative_kernel::modifyScene(scene_data);

	emit onSplitSuccess();
}