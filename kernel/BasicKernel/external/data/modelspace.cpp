#include "modelspace.h"

#include "data/modeln.h"
#include "kernel/kernel.h"
#include "kernel/modelnselector.h"

#include "qtusercore/module/systemutil.h"
#include "qtusercore/string/stringtool.h"

#include "internal/render/printerentity.h"
#include "qtusercore/string/resourcesfinder.h"

#include "internal/menu/submenurecentfiles.h"
#include "internal/layout/layoutitemex.h"

#include "interface/reuseableinterface.h"
#include "interface/uiinterface.h"
#include "interface/printerinterface.h"
#include "interface/camerainterface.h"
#include "interface/renderinterface.h"
#include "interface/visualsceneinterface.h"
#include "interface/shareddatainterface.h"
#include "interface/layoutinterface.h"
#include "interface/selectorinterface.h"

#include "qtuser3d/camera/cameracontroller.h"
#include "qtuser3d/trimesh2/conv.h"

#include "internal/multi_printer/printermanager.h"

#include <QtCore>
#include "data/modelspaceundo.h"
#include "kernel/kernel.h"
#include "job/splitmodel2partsjob.h"
#include "cxkernel/interface/jobsinterface.h"

#include "internal/multi_printer/printermanager.h"

using namespace qtuser_3d;
namespace creative_kernel
{
	ModelSpace::ModelSpace(QObject* parent)
		:QObject(parent)
		, m_spaceDirty(true)
		, m_space_undo(nullptr)
		, m_shared_data_manager(nullptr)
		, m_model_selector(nullptr)
	{
		m_baseBoundingBox += trimesh::dvec3(0.0, 0.0, 0.0);
		m_baseBoundingBox += trimesh::dvec3(300.0, 300.0, 300.0);

		// to compatible with 3mf, begin with "1"
		m_uniqueID = 1;

	}

	ModelSpace::~ModelSpace()
	{
#ifndef Q515
		delete m_root;
#endif
	}

	void ModelSpace::initialize()
	{
	}

	void ModelSpace::uninitialize()
	{
	}

	void ModelSpace::addSpaceTracer(SpaceTracer* tracer)
	{
		if (tracer)
		{
			tracer->onBoxChanged(m_baseBoundingBox);
			m_spaceTracers.push_back(tracer);
		}
	}

	void ModelSpace::removeSpaceTracer(SpaceTracer* tracer)
	{
		if(tracer)
			m_spaceTracers.removeAt(m_spaceTracers.indexOf(tracer));
	}

	void ModelSpace::setSpaceUndo(ModelSpaceUndo* space_undo)
	{
		m_space_undo = space_undo;
	}

	void ModelSpace::setSharedDataManager(SharedDataManager* shared_data_manager)
	{
		m_shared_data_manager = shared_data_manager;
	}

	void ModelSpace::setModelSelector(ModelNSelector* model_selector)
	{
		m_model_selector = model_selector;
	}

	trimesh::dbox3 ModelSpace::getBaseBoundingBox()
	{
		return m_baseBoundingBox;
	}

	trimesh::dbox3 ModelSpace::sceneBoundingBox()
	{
		trimesh::dbox3 box;
		QList<ModelN*> models = modelns();
		for (ModelN* m : models)
		{
			auto appendBox = m->globalBoundingBox();
			box += appendBox;
		}
		return box;
	}

	void ModelSpace::setBaseBoundingBox(const trimesh::dbox3& box)
	{
		if (m_baseBoundingBox == box)
			return;

		m_baseBoundingBox = box;
		for (SpaceTracer* tracer : m_spaceTracers)
			tracer->onBoxChanged(m_baseBoundingBox);
	}

	QList<ModelN*> ModelSpace::modelns()
	{
		QList<ModelN*> models;
		QList<ModelGroup*> groups = modelGroups();
		for(ModelGroup* group : groups)
		{
			models << group->models();
		}
		return models;
	}

	QString ModelSpace::mainModelName()
	{
		if (!haveModelN())
		{
			return QString();
		}

		QList<ModelN*> models = m_model_selector->selectionms();
		if (models.isEmpty())
			models = modelns();
		
		ModelN* model = models[0];
		QFileInfo file(model->objectName());
		if (file.exists())
		{
			return file.completeBaseName();
		}
		else {
			return model->name();
		}
		
	}

	int ModelSpace::modelNNum()
	{
		return modelns().size();
	}

	bool ModelSpace::haveModelN()
	{
		return modelns().size() > 0;
	}

	void ModelSpace::mergeGroups(const QList<ModelGroup*>& groups)
	{
		if (groups.size() <= 0)
			return;

		// set new modelGroup global transform
		qtuser_3d::Box3D gbox;
		for (int i = 0; i < groups.size(); i++)
		{
			ModelGroup* aGroup = groups.at(i);
			if (!aGroup)
				continue;

			gbox += aGroup->globalBox();
		}

		QVector3D newBoxCenter = gbox.center();

		QMatrix4x4 groupXf;
		groupXf.setToIdentity();
		groupXf.translate(newBoxCenter.x(), newBoxCenter.y(), gbox.size().z()/2.0f);
		
		QMatrix4x4 groupZOffsetMatrix;
		if(gbox.min.z() < 0.0f)
		{
			float newZPos = -gbox.min.z();
			groupZOffsetMatrix.setToIdentity();
			groupZOffsetMatrix.translate(0.0f, 0.0f, newZPos);
		}

		creative_kernel::SceneCreateData scene_create_data;

		GroupCreateData group_create_data;  // create one merge group
		group_create_data.xf = qtuser_3d::qMatrix2Xform(groupXf);
		group_create_data.model_group_id = -1;  // create a new group
		group_create_data.name = tr("Group");  

		QList<GroupCreateData> group_remove_datas;  // remove the previous groups

		QList<ModelGroup*> sortedGroups = groups;
		std::sort(sortedGroups.begin(), sortedGroups.end(), [](ModelGroup* g1, ModelGroup* g2)->bool {
			int64_t id1 = g1->getObjectId(), id2 = g2->getObjectId();
			return id1 < id2;
		});

		int partIndex = 0;
		for (int k = 0; k < sortedGroups.size(); k++)
		{
			GroupCreateData one_remove_group_data;
			one_remove_group_data.reset();

			ModelGroup* aGroup = sortedGroups.at(k);
			if (!aGroup)
				continue;

			one_remove_group_data.model_group_id = aGroup->getObjectId();
			one_remove_group_data.name = aGroup->groupName();
			one_remove_group_data.xf = aGroup->getMatrix();

			int modelSize = aGroup->models().size();
			for (int i = 0; i < modelSize; i++)
			{
				ModelN* model = aGroup->models().at(i);
				if (model)
				{
					ModelCreateData model_create_data;
					model_create_data.model_id = -1;
					model_create_data.fixed_id = model->getFixedId();
					model_create_data.dataID = model->sharedDataID();
					model_create_data.usettings = model->setting()->copy();
					model_create_data.reliefDescription = model->fontMeshString();
					model_create_data.name = model->name();
					//model_create_data.layerHeightProfile = model->layerHeightProfile();

					model_create_data.model_part_index = partIndex;
					partIndex++;

					QMatrix4x4 mlmat = model->localMatrix();
					mlmat = mlmat * groupZOffsetMatrix;

					QMatrix4x4 localxf = groupXf.inverted() * aGroup->globalMatrix() * mlmat;
					model_create_data.xf = qtuser_3d::qMatrix2Xform(localxf);

					group_create_data.models.append(model_create_data);

					ModelCreateData model_remove_data;
					model_remove_data.model_id = model->getObjectId();
					model_remove_data.fixed_id = model->getFixedId();
					model_remove_data.dataID = model->sharedDataID();
					model_remove_data.name = model->name();

					model_remove_data.xf = model->getMatrix();

					

					one_remove_group_data.models.append(model_remove_data);

				}

			}

			group_remove_datas.append(one_remove_group_data);

		}

		scene_create_data.add_groups.append(group_create_data);
		scene_create_data.remove_groups.append(group_remove_datas);

		modifyScene(scene_create_data);
	}

	void ModelSpace::splitGroup2Objects(ModelGroup* _model_group)
	{
		if (!_model_group)
			return;

		if (_model_group->normalPartCount() == 1)
		{
			ModelN* aModel = getModelNByObjectId(_model_group->firstNormalPartObjectId());
			if(aModel)
				splitModel2Objects(aModel);

			return;
		}

		creative_kernel::SceneCreateData scene_create_data;

		GroupCreateData group_remove_data;  // remove the selected modelGroup
		group_remove_data.name = _model_group->groupName();
		group_remove_data.model_group_id = _model_group->getObjectId();
		group_remove_data.xf = trimesh::xform(qtuser_3d::qMatrix2Xform(_model_group->localMatrix()));

		QList<GroupCreateData> group_create_datas;  // add some modelGroups according to models in the selected modelGroup

		int modelSize = _model_group->models().size();

		for (int i = 0; i < modelSize; i++)
		{
			ModelN* model = _model_group->models().at(i);

			if (model->modelNType() != ModelNType::normal_part)
			{
				ModelCreateData model_remove_data;
				model_remove_data.model_id = model->getObjectId();
				model_remove_data.fixed_id = model->getFixedId();
				model_remove_data.dataID = model->sharedDataID();
				group_remove_data.models.append(model_remove_data);
				continue;
			}

			if (model)
			{
				GroupCreateData one_group_data;
				one_group_data.reset();

				ModelCreateData model_remove_data;
				model_remove_data.model_id = model->getObjectId();
				model_remove_data.fixed_id = model->getFixedId();
				model_remove_data.dataID = model->sharedDataID();
				group_remove_data.models.append(model_remove_data);

				ModelCreateData model_create_data;
				model_create_data.model_id = -1;
				model_create_data.fixed_id = model->getFixedId();
				model_create_data.name = model->name();

				model_create_data.xf = trimesh::xform::identity();

				model_create_data.dataID = model->sharedDataID();
				model_create_data.usettings = model->setting()->copy();
						model_create_data.reliefDescription = model->fontMeshString();
				model_create_data.layerHeightProfile = model->layerHeightProfile();

				one_group_data.models.append(model_create_data);

				one_group_data.xf = qtuser_3d::qMatrix2Xform(model->globalMatrix());

				one_group_data.usettings = _model_group->setting()->copy();
				one_group_data.name = model->name();

				group_create_datas.append(one_group_data);

			}

		}

		scene_create_data.add_groups.append(group_create_datas);
		scene_create_data.remove_groups.append(group_remove_data);

		modifyScene(scene_create_data);
	}

	void ModelSpace::splitModel2Parts(ModelN* _model)
	{
		SplitModel2PartsJob* splitjob = new SplitModel2PartsJob();
		splitjob->setModel(_model);
		splitjob->setSplit2ObjectFlag(false);
		cxkernel::executeJob(splitjob);
	}

	/*
	* split model mesh into different objects; the model mesh can be splited, and the belonging modelGroup only have one model
	*/
	void ModelSpace::splitModel2Objects(ModelN* _model)
	{
		SplitModel2PartsJob* splitjob = new SplitModel2PartsJob();
		splitjob->setModel(_model);
		splitjob->setSplit2ObjectFlag(true);
		cxkernel::executeJob(splitjob);
	}

	void ModelSpace::cloneGroups(const QList<ModelGroup*>& selGroups, int num)
	{
		if (selGroups.size() <= 0 || num <= 0)
			return;

		std::vector<trimesh::dvec3> centers;
		QList<CONTOUR_PATH> groupOutlinePaths;
		QList<QVector3D> layoutPos;
		for (int k = 0; k < selGroups.size(); k++)
		{
			ModelGroup* aGroup = selGroups.at(k);
			if (!aGroup)
				continue;

			for (int i = 0; i < num; ++i)
			{
				creative_kernel::LayoutItemEx layoutItem(aGroup->getObjectId());
				groupOutlinePaths.push_back(layoutItem.outLine());
			}

			centers.push_back(aGroup->globalBoundingBox().center());
		}

		getPositionBySimpleLayout(groupOutlinePaths, layoutPos);


		creative_kernel::SceneCreateData scene_create_data;
		QList<GroupCreateData> copy_group_datas;

		for (int k = 0; k < selGroups.size(); k++)
		{
			ModelGroup* aGroup = selGroups.at(k);
			if (!aGroup)
				continue;

			trimesh::xform group_matrix = aGroup->getMatrix();
			for (int i = 0; i < num; ++i)
			{
				GroupCreateData one_copy_group_data;
				one_copy_group_data.reset();

				trimesh::dvec3 offset(0.0, 0.0, 0.0);
				int tmpIndex = num * k + i;
				if (tmpIndex >= 0 && tmpIndex < layoutPos.size())
				{
					offset.x = (layoutPos[tmpIndex].x());
					offset.y = (layoutPos[tmpIndex].y());
				}

				trimesh::xform xf = trimesh::xform::trans(offset) * group_matrix;

				one_copy_group_data.xf = xf;
				one_copy_group_data.name = aGroup->groupName();
				one_copy_group_data.usettings = aGroup->setting()->copy();
				one_copy_group_data.outlinePath = aGroup->rsPath();
				one_copy_group_data.outlinePathInitPos = aGroup->getNodeOffset();

				int modelSize = aGroup->models().size();

				for (int j = 0; j < modelSize; j++)
				{
					ModelN* model = aGroup->models().at(j);
					if (model)
					{
						ModelCreateData model_create_data;
						model_create_data.model_id = -1;
						model_create_data.fixed_id = model->getFixedId();
						model_create_data.dataID = model->sharedDataID();
						model_create_data.name = model->name();

						model_create_data.xf = model->getMatrix();
						model_create_data.usettings = model->setting()->copy();
						model_create_data.reliefDescription = model->fontMeshString(true);
						model_create_data.layerHeightProfile = model->layerHeightProfile();

						one_copy_group_data.models.append(model_create_data);
					}
				}

				copy_group_datas.append(one_copy_group_data);

			}
		}

		scene_create_data.add_groups = copy_group_datas;
		modifyScene(scene_create_data);
	}

	void ModelSpace::cloneModels(const QList<ModelN*>& selModels, int num)
	{
		if (selModels.size() <= 0)
			return;

		ModelGroup* theGroup = selModels[0]->getModelGroup();
		if (!theGroup)
			return;

		QList<ModelCreateData> modelCreateDatas;

		for (ModelN* model : selModels)
		{
			qtuser_3d::Box3D mbox = model->globalSpaceBox();
			float xOffset = mbox.size().x() + 1.0f;
			trimesh::xform offsetXf = trimesh::xform::trans(xOffset, 0.0f, 0.0f);

			for (int i = 0; i < num; ++i)
			{
				ModelCreateData model_create_data;
				model_create_data.model_id = -1;
				model_create_data.fixed_id = -1;
				model_create_data.dataID = model->sharedDataID();
				model_create_data.name = model->name();

				model_create_data.xf = model->getMatrix() * offsetXf;
				model_create_data.usettings = model->setting()->copy();
				model_create_data.reliefDescription = model->fontMeshString(true);
				model_create_data.layerHeightProfile = model->layerHeightProfile();

				modelCreateDatas.append(model_create_data);
			}

		}

		addModels(modelCreateDatas, theGroup);
	}

	void ModelSpace::addPart2Group(ModelDataPtr data, ModelNType mtype, ModelGroup* _model_group)
#if (0)
	{
		if (!_model_group || !data.get())
			return;

		// set new modelGroup global transform
		qtuser_3d::Box3D gbox = _model_group->globalBox();
		float zmin = gbox.min.z();
		float xmax = gbox.max.x();
		float ymin = gbox.min.y();
		float ymax = gbox.max.y();

		trimesh::dbox3 localMeshBox = data->mesh->calculateBox();
		float xsize = localMeshBox.size().x;
		float ysize = localMeshBox.size().y;
		float zsize = localMeshBox.size().z;

		QMatrix4x4 meshOffMat;
		meshOffMat.setToIdentity();
		const float meshOffXVal = 1.0f;
		meshOffMat.translate(xmax+xsize/2.0f+ meshOffXVal, ymin+ysize/2.0f  ,zmin+zsize/2.0f );
		trimesh::dbox3 meshGlobalBox = data->mesh->calculateBox(trimesh::xform(qtuser_3d::qMatrix2Xform(meshOffMat)));
		trimesh::box3 fgbox;
		fgbox.min = meshGlobalBox.min;
		fgbox.max = meshGlobalBox.max;

		qtuser_3d::Box3D newGBox = gbox + qtuser_3d::triBox2Box3D(fgbox);

		// set new global transform
		QMatrix4x4 newGroupXf;
		newGroupXf.setToIdentity();
		newGroupXf.translate(newGBox.center().x(), newGBox.center().y(), newGBox.size().z() / 2.0f);

		QMatrix4x4 groupZOffsetMatrix;
		groupZOffsetMatrix.setToIdentity();
		if (newGBox.min.z() < 0.0f)
		{
			float newZPos = -newGBox.min.z();
			groupZOffsetMatrix.translate(0.0f, 0.0f, newZPos);
		}

		SceneCreateData scene_data;

		GroupCreateData group_create_data;
		group_create_data.model_group_id = _model_group->getObjectId();
		group_create_data.xf = _model_group->getMatrix();

		ModelCreateData mesh_model_data;

		mesh_model_data.dataID = registerModelData(data);
		mesh_model_data.dataID(ModelType) = (int)mtype;
		mesh_model_data.dataID(MaterialID) = _model_group->defaultColorIndex();
		mesh_model_data.name = data->name;

		QMatrix4x4 gXf = _model_group->localMatrix();
		mesh_model_data.xf = qtuser_3d::qMatrix2Xform(gXf.inverted() * meshOffMat);

		group_create_data.models.append(mesh_model_data);

		scene_data.add_groups.append(group_create_data);

		modifyScene(scene_data);
	}
#else
	{
		if (!_model_group || !data.get())
			return;

		// set new modelGroup global transform
		qtuser_3d::Box3D gbox = _model_group->globalBox();
		float zmin = gbox.min.z();
		float xmax = gbox.max.x();
		float ymin = gbox.min.y();
		float ymax = gbox.max.y();

		trimesh::dbox3 localMeshBox = data->mesh->calculateBox();
		float xsize = localMeshBox.size().x;
		float ysize = localMeshBox.size().y;
		float zsize = localMeshBox.size().z;

		QMatrix4x4 meshOffMat;
		meshOffMat.setToIdentity();
		const float meshOffXVal = 1.0f;
		meshOffMat.translate(xmax + xsize / 2.0f + meshOffXVal, ymin + ysize / 2.0f, zmin + zsize / 2.0f);
		trimesh::dbox3 meshGlobalBox = data->mesh->calculateBox(trimesh::xform(qtuser_3d::qMatrix2Xform(meshOffMat)));
		trimesh::box3 fgbox;
		fgbox.min = meshGlobalBox.min;
		fgbox.max = meshGlobalBox.max;

		qtuser_3d::Box3D newGBox = gbox + qtuser_3d::triBox2Box3D(fgbox);

		// set new global transform
		QMatrix4x4 newGroupXf;
		newGroupXf.setToIdentity();
		newGroupXf.translate(newGBox.center().x(), newGBox.center().y(), newGBox.size().z() / 2.0f);

		QMatrix4x4 groupZOffsetMatrix;
		groupZOffsetMatrix.setToIdentity();
		if (newGBox.min.z() < 0.0f)
		{
			float newZPos = -newGBox.min.z();
			groupZOffsetMatrix.translate(0.0f, 0.0f, newZPos);
		}


		GroupCreateData group_create_data;  // create one merge group
		group_create_data.xf = qtuser_3d::qMatrix2Xform(newGroupXf);
		group_create_data.model_group_id = -1;  // create a new group
		group_create_data.name = _model_group->groupName();

		GroupCreateData group_remove_data;
		group_remove_data.reset();
		group_remove_data.model_group_id = _model_group->getObjectId();
		group_remove_data.name = _model_group->groupName();
		group_remove_data.xf = _model_group->getMatrix();

		int modelSize = _model_group->models().size();
		for (int i = 0; i < modelSize; i++)
		{
			ModelN* model = _model_group->models().at(i);
			if (model)
			{
				ModelCreateData model_remove_data;
				model_remove_data.model_id = model->getObjectId();
				model_remove_data.fixed_id = model->getFixedId();
				model_remove_data.dataID = model->sharedDataID();
				model_remove_data.name = model->name();
				model_remove_data.xf = model->getMatrix();
				
				group_remove_data.models.append(model_remove_data);


				ModelCreateData model_create_data;
				model_create_data.model_id = -1;
				model_create_data.fixed_id = model->getFixedId();
				model_create_data.dataID = model->sharedDataID();
				model_create_data.usettings = model->setting()->copy();
				model_create_data.name = model->name();
						model_create_data.reliefDescription = model->fontMeshString();

				QMatrix4x4 mlmat = model->localMatrix();
				mlmat = mlmat * groupZOffsetMatrix;
				QMatrix4x4 localxf = newGroupXf.inverted() * _model_group->globalMatrix() * mlmat;
				model_create_data.xf = qtuser_3d::qMatrix2Xform(localxf);

				group_create_data.models.append(model_create_data);

			}
		}

		ModelCreateData mesh_model_data;
		mesh_model_data.dataID = registerModelData(data);
		mesh_model_data.dataID(ModelType) = (int)mtype;
		mesh_model_data.dataID(MaterialID) = _model_group->defaultColorIndex();
		mesh_model_data.name = data->name;
		mesh_model_data.xf = qtuser_3d::qMatrix2Xform(newGroupXf.inverted() * meshOffMat);

		group_create_data.models.append(mesh_model_data);

		creative_kernel::SceneCreateData scene_data;
		scene_data.add_groups.append(group_create_data);
		scene_data.remove_groups.append(group_remove_data);

		modifyScene(scene_data);
	}
#endif


	void ModelSpace::uniformName(ModelN* item)
	{
		if (!item)
			return;

		QList<ModelN*> items = modelns();
		bool hasSame = false;
		for (auto itemData : items)
		{
			if (itemData->objectName() == item->objectName())
			{
				hasSame = true;
				break;
			}
		}

		if (!hasSame)
			return;
		
		QString objectName = item->objectName();
		QFileInfo file(objectName);
		QString suffix = file.suffix().isEmpty() ? "" : "." + file.suffix();
		QString baseName = file.baseName();

		item->setProperty("baseName", objectName);
		item->setProperty("baseNameID", objectName);

		int nameIndex = 1;
		QString name = QString("%1-%2").arg(baseName).arg(nameIndex) + suffix;
		//---                
		QList<ModelN*> models = modelns();
		QString sname;
		for (int k = 0; k < models.size(); ++k)
		{
			sname = models[k]->objectName();
			if (name == sname)
			{
				nameIndex++;
				name = QString("%1-%2").arg(baseName).arg(nameIndex) + suffix;
				k = -1;
			}
		}
		//---
		item->setObjectName(name);

	}

	QList<ModelGroup*> ModelSpace::modelGroups()
	{
		return m_sceneModelGroups;
	}

	void ModelSpace::setSpaceDirty(bool _spaceDirty)
	{
		m_spaceDirty = _spaceDirty;
	}

	bool ModelSpace::spaceDirty()
	{
		return m_spaceDirty;
	}

	bool ModelSpace::haveModelsOutPlatform()
	{
		bool onPlatformBorder = false;
		for (int i = 0, count = creative_kernel::printersCount(); i < count; ++i)
		{
			auto p = creative_kernel::getPrinter(i);
			if (p->hasModelsOnBorder())
			{
				auto modelsOnBorder = p->modelsOnBorder();
				float minX, minY, minZ, maxX, maxY, maxZ;
				minX = minY = minZ = 100000;
				maxX = maxY = maxZ = -100000;
				for (auto m : modelsOnBorder)
				{
					// 计算预览区域
					auto box = m->globalSpaceBox();
					minX = box.min.x() < minX ? box.min.x() : minX;
					minY = box.min.y() < minY ? box.min.y() : minY;
					minZ = box.min.z() < minZ ? box.min.z() : minZ;
					maxX = box.max.x() > maxX ? box.max.x() : maxX;
					maxY = box.max.y() > maxY ? box.max.y() : maxY;
					maxZ = box.max.z() > maxZ ? box.max.z() : maxZ;
				}
				qtuser_3d::Box3D box(QVector3D(minX, minY, minZ), QVector3D(maxX, maxY, maxZ));
				QVector3D dir(0.0f, 1.0f, -1.0f);
				QVector3D right(1.0f, 0.0f, 0.0f);
				QVector3D center = box.center();
				center.setZ(0);
				creative_kernel::cameraController()->view(dir, right, &center);
				onPlatformBorder = true;
			}
		}

		return onPlatformBorder;
	}

	bool ModelSpace::modelOutPlatform(ModelN* amode)
	{
		QVector3D bbOffset(0.1f, 0.1f, 0.1f);
		Box3D baseBox;
		baseBox += QVector3D(m_baseBoundingBox.min.x, m_baseBoundingBox.min.y, m_baseBoundingBox.min.z);
		baseBox += QVector3D(m_baseBoundingBox.max.x, m_baseBoundingBox.max.y, m_baseBoundingBox.max.z);

		baseBox.min = baseBox.min - bbOffset;
		baseBox.max = baseBox.max + bbOffset;

		qtuser_3d::Box3D LocalBox = amode->globalSpaceBox();
		LocalBox.min.setZ(0.0);
		LocalBox.max.setZ(0.0);
		bool outPlatform = false;
		if (!baseBox.contains(LocalBox))
		{
			outPlatform = true;
		}
		return outPlatform;
	}

	ModelGroup* ModelSpace::getModelGroupByObjectId(int objId)
	{
		for (ModelGroup* mg : m_sceneModelGroups)
		{
			if (objId == mg->getObjectId())
				return mg;
		}

		return nullptr;
	}

	ModelN* ModelSpace::getModelNByObjectId(int objId)
	{
		for (ModelGroup* mg : m_sceneModelGroups)
		{
			ModelN* model = mg->getModelNByObjectId(objId);
			if (model)
				return model;
		}

		return nullptr;
	}
	
	ModelN* ModelSpace::getModelNByFixedId(int fixedId)
	{
		for (ModelGroup* mg : m_sceneModelGroups)
		{
			ModelN* model = mg->getModelNByFixedId(fixedId);
			if (model)
				return model;
		}

		return nullptr;
	}

	void ModelSpace::beginNodeSnap(const QList<ModelGroup*>& groups, const QList<ModelN*>& models)
	{
		m_nodes_snap.clear();

		if (groups.size() == 0 && models.size() == 0)
			return;

		for (ModelGroup* group : groups)
		{
			NodeChange nc;
			nc.id = group->getObjectId();
			nc.start_xf = group->getMatrix();
			nc.end_xf = trimesh::xform::identity();
			m_nodes_snap.push_back(nc);
		}

		for (ModelN* model : models)
		{
			NodeChange nc;
			nc.id = model->getObjectId();
			nc.start_xf = model->getMatrix();
			nc.end_xf = trimesh::xform::identity();
			m_nodes_snap.push_back(nc);
		}
	}

	void ModelSpace::endNodeSnap()
	{
		QList<NodeChange> real_node_change;
		for (NodeChange& nc : m_nodes_snap)
		{
			ModelGroup* group = getModelGroupByObjectId(nc.id);
			if (group)
			{
				nc.end_xf = group->getMatrix();
				real_node_change.push_back(nc);
				continue;
			}

			ModelN* model = getModelNByObjectId(nc.id);

			if (model)
			{
				nc.end_xf = model->getMatrix();
				real_node_change.push_back(nc);
				continue;
			}
		}

		ModelSpaceUndo* stack = getKernel()->modelSpaceUndo();
		stack->modifyNodes(real_node_change);
		m_nodes_snap.clear();
	}

	void ModelSpace::runSnap(const QList<ModelGroup*>& groups, const QList<ModelN*>& models, std::function<void()> func, bool snap)
	{
		if(snap)
			beginNodeSnap(QList<ModelGroup*>(), models);
		if (func)
			func();
		if(snap)
			endNodeSnap();
	}

	void ModelSpace::modifyScene(const SceneCreateData& scene_data)
	{
		ModelSpaceUndo* stack = getKernel()->modelSpaceUndo();
		stack->modifySpace(scene_data);
	}

	void ModelSpace::clearSpace()
	{
		deleteModelGroups(m_sceneModelGroups);
	}

	void ModelSpace::addModel(const ModelCreateData& model_data, ModelGroup* model_group)
	{
		if (!model_group)
			return;

		QList<ModelCreateData> model_datas;
		model_datas << model_data;
		addModels(model_datas, model_group);
	}

	void ModelSpace::addModels(const QList<ModelCreateData>& model_datas, ModelGroup* model_group)
	{
		SceneCreateData scene_data;
		GroupCreateData group_data;
		group_data.models = model_datas;
		group_data.model_group_id = model_group ? model_group->getObjectId() : -1;
		if (group_data.model_group_id >= 0 && model_group)
		{
			group_data.xf = model_group->getMatrix();
		}
		scene_data.add_groups.append(group_data);
		modifyScene(scene_data);
	}

	void ModelSpace::deleteModelGroup(ModelGroup* _model_group)
	{
		if (!_model_group)
			return;

		QList<ModelGroup*> _model_groups;
		_model_groups << _model_group;
		deleteModelGroups(_model_groups);
	}

	void ModelSpace::deleteModelGroups(const QList<ModelGroup*>& _model_groups)
	{
		if (_model_groups.size() == 0)
			return;

		SceneCreateData scene_data;
		for (ModelGroup* _model_group : _model_groups)
		{
			scene_data.remove_groups.append(generateGroupCreateData(_model_group));
		}
		modifyScene(scene_data);
	}

	//models must have same group
	void ModelSpace::deleteModel(ModelN* model)
	{
		if (!model)
			return;

		QList<ModelN*> _models;
		_models << model;
		deleteModels(_models);
	}

	void ModelSpace::deleteModels(const QList<ModelN*>& models)
	{
		int size = models.size();
		if (size <= 0)
			return;
		
		ModelGroup* only_group = models.at(0)->getModelGroup();
		if (!only_group)
		{
			qDebug() << QString("deleteModelsFromModelGroup error.");
			return;
		}

		for (int i = 1; i < size; ++i)
		{
			if (only_group != models.at(i)->getModelGroup())
			{
				qDebug() << QString("deleteModelsFromModelGroup group unsame.");
				return;
			}
		}

		if (only_group->normalPartCount() <= 1)
		{
			for (int i = 0; i < models.size(); i++)
			{
				if (models[i]->modelNType() == ModelNType::normal_part)
				{
					qDebug() << QStringLiteral("can not delete normal part model if the group has only one normal_part model inside");
					return;
				}
			}

		}

		SceneCreateData scene_data;
		GroupCreateData group_data;
		group_data.model_group_id = only_group->getObjectId();

		if (only_group->models().size() - models.size() == 1)
		{
			for (ModelN* m : only_group->models())
			{
				if (!models.contains(m))
				{
					only_group->setGroupName(m->name());
					break;
				}
			}
		}

		for (ModelN* _model : models)
		{
			group_data.models.append(generateModelCreateData(_model));
		}

		scene_data.remove_groups.append(group_data);
		modifyScene(scene_data);
	}

	void ModelSpace::deleteUnion(const QList<ModelGroup*>& _model_groups, const QList<ModelN*>& models)
	{
		QList<ModelN*> real_models;
		for (ModelN* model : models)
		{
			if (_model_groups.contains(model->getModelGroup()))
				continue;

			real_models.append(model);
		}

		SceneCreateData scene_data;
		for (ModelGroup* model_group : _model_groups)
		{
			scene_data.remove_groups.append(generateGroupCreateData(model_group));
		}
		for (ModelN* model : real_models)
		{
			GroupCreateData group_data;
			group_data.model_group_id = model->getModelGroup()->getObjectId();
			group_data.models.append(generateModelCreateData(model));
			scene_data.remove_groups.append(group_data);
		}

		modifyScene(scene_data);
	}

	void ModelSpace::addModelGroup_ur(ModelGroup* _model_group)
	{
		if (!_model_group)
			return;

		QList<ModelGroup*> _model_groups;
		_model_groups << _model_group;
		addModelGroups_ur(_model_groups);
	}

	void ModelSpace::addModelGroups_ur(const QList<ModelGroup*>& _model_groups)
	{
		if (_model_groups.size() == 0)
			return;

		for (ModelGroup* _model_group : _model_groups)
		{
			if (!_model_group || m_sceneModelGroups.contains(_model_group))
			{
				qDebug() << QString("addModelGroup_ur already in.");
				continue;
			}
			
			_model_group->setParent(this);
			m_sceneModelGroups.append(_model_group);
			connect(_model_group, &ModelGroup::modelNTypeChanged, this, &ModelSpace::onModelNTypeChanged, Qt::UniqueConnection);

			//crslice2
			crslice2::CrSliceObject* object = m_crslice_model.add_object();
			_model_group->m_crslice_object = object;


			for (SpaceTracer* tracer : m_spaceTracers)
			{
				tracer->onModelGroupAdded(_model_group);
			}

		}
		checkModelGroupSupportGenerator();

		std::sort(m_sceneModelGroups.begin(), m_sceneModelGroups.end(), [](ModelGroup* mg1, ModelGroup* mg2)->bool {
			return mg1->getObjectId() < mg2->getObjectId();
			});
	}

	void ModelSpace::removeModelGroup_ur(ModelGroup* _model_group)
	{
		if (!_model_group)
			return;

		QList<ModelGroup*> _model_groups;
		_model_groups << _model_group;
		removeModelGroups_ur(_model_groups);
	}

	void ModelSpace::removeModelGroups_ur(const QList<ModelGroup*>& _model_groups)
	{
		if (_model_groups.size() == 0)
			return;

		m_model_selector->modifyModelGroups(_model_groups);
		VisualScene* scene = getKernel()->visScene();

		for (ModelGroup* _model_group : _model_groups)
		{
			if (!m_sceneModelGroups.contains(_model_group))
			{
				qDebug() << QString("addModelGroup_ur not in.");
				continue;
			}

			for (auto iter = m_spaceTracers.cbegin(); iter != m_spaceTracers.cend(); ++iter)
			{
				(*iter)->onModelGroupRemoved(_model_group);
			}

			_model_group->setParent(nullptr);
			m_sceneModelGroups.removeOne(_model_group);
			
			//crslice2
			m_crslice_model.remove_object(_model_group->m_crslice_object);

			disconnect(_model_group, &ModelGroup::modelNTypeChanged, this, &ModelSpace::onModelNTypeChanged);
		}

		checkModelGroupSupportGenerator();

		std::sort(m_sceneModelGroups.begin(), m_sceneModelGroups.end(), [](ModelGroup* mg1, ModelGroup* mg2)->bool {
			return mg1->getObjectId() < mg2->getObjectId();
			});

	}

	void ModelSpace::addModel_ur(ModelN* model, ModelGroup* _model_group)
	{
		if (!model || !_model_group)
			return;

		QList<ModelN*> models;
		models << model;
		addModels_ur(models, _model_group);
	}

	void ModelSpace::addModels_ur(const QList<ModelN*>& models, ModelGroup* _model_group)
	{
		if (!_model_group)
			return;

		if (models.size() == 0)
			return;

		_model_group->addModels(models);

		QList<ModelN*> removedModels;
		for (SpaceTracer* tracer : m_spaceTracers)
		{
			tracer->onModelGroupModified(_model_group, removedModels, models);
		}
		onModelGroupModified(_model_group, removedModels, models);

		m_model_selector->modifyModels(models, QList<ModelN*>());

	}

	void ModelSpace::removeModel_ur(ModelN* model, ModelGroup* _model_group)
	{
		if (!model || !_model_group)
			return;

		QList<ModelN*> models;
		models << model;
		removeModels_ur(models, _model_group);
	}

	void ModelSpace::removeModels_ur(const QList<ModelN*>& models, ModelGroup* _model_group)
	{
		if (!_model_group)
			return;

		if (models.size() == 0)
			return;

		VisualScene* scene = getKernel()->visScene();

		_model_group->removeModels(models);

		QList<ModelN*> addModels;
		for (SpaceTracer* tracer : m_spaceTracers)
		{
			tracer->onModelGroupModified(_model_group, models, addModels);
		}
		onModelGroupModified(_model_group, models, addModels);

		m_model_selector->modifyModels(QList<ModelN*>(), models);
	}

	ModelN* ModelSpace::createModel(QObject* parent, int64_t fixedId, int64_t id)
	{
		ModelN* model = nullptr;
		if (id == -1)
		{
			model = new ModelN(parent);
			model->setObjectId(generateUniqueId());
		}
		else {  //model_group_id >= 0
			model = getModelNByObjectId(id);
			if (!model)
			{
				model = new ModelN(parent);
				model->setObjectId(id);
			}
		}

		if (fixedId == -1)
		{
			model->setFixedId(generateFixedId());
		}
		else 
		{
			model->setFixedId(fixedId);
		}

		return model;
	}

	ModelGroup* ModelSpace::createModelGroup(QObject* parent, int64_t id)
	{
		ModelGroup * _model_group = nullptr;
		if (id == -1)
		{
			_model_group = new ModelGroup(parent);
			_model_group->setObjectId(generateUniqueId());
			connect(_model_group, &ModelGroup::supportEnabledChanged, this, &ModelSpace::onSupportEnabledChanged, Qt::UniqueConnection);
		}
		else {  //model_group_id >= 0
			_model_group = getModelGroupByObjectId(id);
			if (!_model_group)
			{
				_model_group = new ModelGroup(parent);
				_model_group->setObjectId(id);
				connect(_model_group, &ModelGroup::supportEnabledChanged, this, &ModelSpace::onSupportEnabledChanged, Qt::UniqueConnection);
			}
		}

		assert(_model_group);
		return _model_group;
	}

}
