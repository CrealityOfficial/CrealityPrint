#include "modelspace.h"
#include "data/modeln.h"
#include "data/modelspaceundo.h"
#include "data/spaceutils.h"
#include "data/shareddatamanager/shareddatamanager.h"
#include "kernel/kernel.h"
#include "kernel/modelnselector.h"

#include "interface/selectorinterface.h"
#include "interface/visualsceneinterface.h"
#include "interface/projectinterface.h"
#include "interface/reuseableinterface.h"
#include "interface/modelgroupinterface.h"

#include "qtuser3d/trimesh2/conv.h"
#include "qtuser3d/math/angles.h"
#include "msbase/data/quaterniond.h"

namespace creative_kernel
{
	void ModelSpace::mirrorX(ModelGroup* _model_group, bool reversible)
	{
		if (!_model_group)
			return;

		trimesh::xform mirMatrix;
		mirMatrix(0, 0) = -1.0f;

		trimesh::dvec3 c = _model_group->globalBoundingBox().center();
		trimesh::xform xf = trimesh::xform::trans(c) * mirMatrix * trimesh::xform::trans(-c) ;
		_model_group->applyMatrix(xf);
	}

	void ModelSpace::mirrorY(ModelGroup* _model_group, bool reversible)
	{
		if (!_model_group)
			return;
		
		trimesh::xform mirMatrix;
		mirMatrix(1, 1) = -1.0f;

		trimesh::dvec3 c = _model_group->globalBoundingBox().center();
		trimesh::xform xf = trimesh::xform::trans(c) * mirMatrix * trimesh::xform::trans(-c);
		_model_group->applyMatrix(xf);
	}

	void ModelSpace::mirrorZ(ModelGroup* _model_group, bool reversible)
	{
		if (!_model_group)
			return;

		trimesh::xform mirMatrix;
		mirMatrix(2, 2) = -1.0f;

		trimesh::dvec3 c = _model_group->globalBoundingBox().center();
		trimesh::xform xf = trimesh::xform::trans(c) * mirMatrix * trimesh::xform::trans(-c);
		_model_group->applyMatrix(xf);
	}

	void ModelSpace::mirrorX(ModelN* _model, bool reversible)
	{
		if (!_model)
			return;
		ModelGroup* group = _model->getModelGroup();
		
		trimesh::xform mirMatrix;
		mirMatrix(0, 0) = -1.0f;

		trimesh::dvec3 c = _model->globalBoundingBox().center();
		c = trimesh::inv(group->getMatrix()) * c;

		trimesh::xform xf = trimesh::xform::trans(c) * mirMatrix * trimesh::xform::trans(-c);
		_model->applyMatrix(xf);
	}

	void ModelSpace::mirrorY(ModelN* _model, bool reversible)
	{
		if (!_model)
			return;
		ModelGroup* group = _model->getModelGroup();

		trimesh::xform mirMatrix;
		mirMatrix(1, 1) = -1.0f;

		trimesh::dvec3 c = _model->globalBoundingBox().center();
		c = trimesh::inv(group->getMatrix()) * c;

		trimesh::xform xf = trimesh::xform::trans(c) * mirMatrix * trimesh::xform::trans(-c);
		_model->applyMatrix(xf);
	}

	void ModelSpace::mirrorZ(ModelN* _model, bool reversible)
	{
		if (!_model)
			return;
		ModelGroup* group = _model->getModelGroup();

		trimesh::xform mirMatrix;
		mirMatrix(2, 2) = -1.0f;

		trimesh::dvec3 c = _model->globalBoundingBox().center();
		c = trimesh::inv(group->getMatrix()) * c;

		trimesh::xform xf = trimesh::xform::trans(c) * mirMatrix * trimesh::xform::trans(-c);
		_model->applyMatrix(xf);
	}

	void ModelSpace::renameModelGroup(ModelGroup* _model_group, const QString& name)
	{

	}

	void ModelSpace::renameModelN(ModelN* _model, const QString& name)
	{

	}

	void ModelSpace::replaceModelNMesh(const ModelNPropertyMeshChange& change, bool reversible)
	{
		QList<ModelNPropertyMeshChange> changes;
		changes << change;
		replaceModelNMeshes(changes, reversible);
	}

	void ModelSpace::replaceModelNMeshes(const QList<ModelNPropertyMeshChange>& changes, bool reversible)
	{
		QList<ModelNPropertyMeshUndo> properties_changes;
		for (const ModelNPropertyMeshChange& change : changes)
		{
			if (!change.model || !change.mesh)
				continue;

			ModelNPropertyMeshUndo property_undo;
			property_undo.model_id = change.model->getObjectId();
			property_undo.start_name = change.model->name();
			property_undo.end_name = change.name;
			property_undo.start_data_id = change.model->sharedDataID();

			SharedDataID id;
			id(MeshID) = m_shared_data_manager->registerMeshData(change.mesh, true);
			id(MaterialID) = property_undo.start_data_id(MaterialID);


			property_undo.end_data_id = id;
			properties_changes.append(property_undo);
		}
		m_space_undo->modifyModelsMeshProperty(properties_changes, reversible);
	}
	
	void ModelSpace::replaceModelsProperty(const QList<ModelNPropertyMeshUndo>& changes, bool reversible)
	{
		m_space_undo->modifyModelsMeshProperty(changes, reversible);
	}

	void ModelSpace::rotateXModels(double angle, bool reversible)
	{
		trimesh::dvec3 axis = trimesh::dvec3(1.0, 0.0, 0.0);
		rotateModels(axis, angle, reversible);
	}

	void ModelSpace::rotateYModels(double angle, bool reversible)
	{
		trimesh::dvec3 axis = trimesh::dvec3(0.0, 1.0, 0.0);
		rotateModels(axis, angle, reversible);
	}

	void ModelSpace::rotateZModels(double angle, bool reversible)
	{
		trimesh::dvec3 axis = trimesh::dvec3(0.0, 0.0, 1.0);
		rotateModels(axis, angle, reversible);
	}

	void ModelSpace::moveXModels(double distance, bool reversible)
	{
		QList<ModelN*> models = selectionms();
		auto f = [&models, distance]() {
			
			for (ModelN* model : models)
			{
				ModelGroup* group = model->getModelGroup();
				trimesh::xform m = trimesh::inv(group->getMatrix());

				trimesh::dvec3 src_w = model->globalBoundingBox().center();
				trimesh::dvec3 dst_w = src_w + trimesh::dvec3(distance, 0.0, 0.0);
				trimesh::dvec3 src_l = m * src_w;
				trimesh::dvec3 dst_l = m * trimesh::dvec3(dst_w);

				trimesh::dvec3 tDelta = dst_l - src_l;
				trimesh::xform xf = trimesh::xform::trans(tDelta);
				model->applyMatrix(xf);
			}
		};

		runSnap(QList<ModelGroup*>(), models, f, reversible);
	}

	void ModelSpace::moveYModels(double distance, bool reversible)
	{
		QList<ModelN*> models = selectionms();
		auto f = [&models, distance]() {
			
			for (ModelN* model : models)
			{
				ModelGroup* group = model->getModelGroup();
				trimesh::xform m = trimesh::inv(group->getMatrix());

				trimesh::dvec3 src_w = model->globalBoundingBox().center();
				trimesh::dvec3 dst_w = src_w + trimesh::dvec3(0.0, distance, 0.0);
				trimesh::dvec3 src_l = m * src_w;
				trimesh::dvec3 dst_l = m * trimesh::dvec3(dst_w);

				trimesh::dvec3 tDelta = dst_l - src_l;
				trimesh::xform xf = trimesh::xform::trans(tDelta);
				model->applyMatrix(xf);
			}
		};

		runSnap(QList<ModelGroup*>(), models, f, reversible);
	}

	void ModelSpace::rotateModels(trimesh::dvec3 axis, double angle, bool reversible)
	{
		trimesh::xform xf = trimesh::xform::rot(angle * M_PI_180, axis);

		QList<ModelGroup*> groups = selectedGroups();
		if (groups.size() > 0)
		{
			beginNodeSnap(groups, QList<ModelN*>());
			for (ModelGroup* group : groups)
			{
				group->rotateByStandardAngle(QVector3D(axis.x, axis.y, axis.z), angle);

				trimesh::xform rot = trimesh::xform::rot(angle * M_PI_180, axis);

				trimesh::xform m = group->getMatrix();
				trimesh::dvec3 c = group->globalBoundingBox().center();

				trimesh::xform xf = trimesh::xform::trans(c) * rot * trimesh::xform::trans(-c) * m;
				group->setMatrix(xf);

				group->layerBottom();
				group->updateSweepAreaPath();
			}

			endNodeSnap();
		}
		else {

			QList<ModelN*> models = selectedParts();
			QList<ModelGroup*> groups = findRelativeGroups(models);
			beginNodeSnap(groups, models);

			for (ModelN* model : models)
			{
				model->rotateByStandardAngle(QVector3D(axis.x, axis.y, axis.z), angle);

				ModelGroup* group = model->getModelGroup();

				trimesh::dvec3 center = model->globalBoundingBox().center();
				trimesh::dvec3 c = trimesh::inv(group->getMatrix()) * center;

				trimesh::xform snap_matrix = model->getMatrix();
				trimesh::xform xf = trimesh::xform::trans(c) * xf * trimesh::xform::trans(-c) * snap_matrix;
				model->setMatrix(xf);
			}

			for (ModelGroup* group : groups)
			{
				group->layerBottom();
			}

			updateSpaceNodeRender(models);
			endNodeSnap();
		}
	}

	void ModelSpace::moveModel(ModelN* model, const QVector3D& start, const QVector3D& end, bool reversible)
	{
		if (!model)
			return;

		QList<ModelN*> models;
		models << model;
		QList<QVector3D> starts;
		starts << start;
		QList<QVector3D> ends;
		ends << end;
		moveModels(models, starts, ends, reversible);
	}

	void ModelSpace::moveModels(const QList<ModelN*>& models, const QList<QVector3D>& starts, const QList<QVector3D>& ends, bool reversible)
	{
		QList<NUnionChangedStruct> changes;

		int mCount = models.size();
		int msCount = starts.size();
		int eCount = ends.size();

		if ((mCount == msCount) && (mCount == eCount))
		{
			for (int i = 0; i < mCount; ++i)
			{
				NUnionChangedStruct change;
				change.modelId = models.at(i)->getObjectId();
				change.posActive = true;
				change.posChange.start = starts.at(i);
				change.posChange.end = ends.at(i);
				changes.push_back(change);
			}
		}
		mixUnions(changes, reversible);
	}

	void ModelSpace::mixTSModel(ModelN* model, const QVector3D& tstart, const QVector3D& tend,
		const QVector3D& sstart, const QVector3D& send, bool reversible)
	{
		if (!model)
			return;

		QList<ModelN*> models;
		models << model;
		QList<QVector3D> tStarts;
		tStarts << tstart;
		QList<QVector3D> tEnds;
		tEnds << tend;
		QList<QVector3D> sStarts;
		sStarts << sstart;
		QList<QVector3D> sEnds;
		sEnds << send;
		mixTSModels(models, tStarts, tEnds, sStarts, sEnds, reversible);
	}

	void ModelSpace::mixTSModels(const QList<ModelN*>& models, const QList<QVector3D>& tStarts, const QList<QVector3D>& tEnds,
		const QList<QVector3D>& sStarts, const QList<QVector3D>& sEnds, bool reversible)
	{
		QList<NUnionChangedStruct> changes;

		int mCount = models.size();
		int tCount = tStarts.size();
		int teCount = tEnds.size();
		int sCount = sStarts.size();
		int seCount = sEnds.size();

		if ((mCount == tCount) && (mCount == teCount) && (mCount == sCount)
			&& (mCount == seCount))
		{
			for (int i = 0; i < mCount; ++i)
			{
				NUnionChangedStruct change;
				change.modelId = models.at(i)->getObjectId();
				change.posActive = true;
				change.posChange.start = tStarts.at(i);
				change.posChange.end = tEnds.at(i);
				change.scaleActive = true;
				change.scaleChange.start = sStarts.at(i);
				change.scaleChange.end = sEnds.at(i);
				changes.push_back(change);
			}
		}
		mixUnions(changes, reversible);
	}

	void ModelSpace::mixTRModel(ModelN* model, const QVector3D& tstart, const QVector3D& tend,
		const QQuaternion& rstart, const QQuaternion& rend, bool reversible)
	{
		if (!model)
			return;

		QList<ModelN*> models;
		models << model;
		QList<QVector3D> tStarts;
		tStarts << tstart;
		QList<QVector3D> tEnds;
		tEnds << tend;
		QList<QQuaternion> rStarts;
		rStarts << rstart;
		QList<QQuaternion> rEnds;
		rEnds << rend;
		mixTRModels(models, tStarts, tEnds, rStarts, rEnds, reversible);
	}

	void ModelSpace::mixTRModels(const QList<ModelN*>& models, const QList<QVector3D>& tStarts, const QList<QVector3D>& tEnds,
		const QList<QQuaternion>& rStarts, const QList<QQuaternion>& rEnds, bool reversible)
	{
		QList<NUnionChangedStruct> changes;

		int mCount = models.size();
		int tCount = tStarts.size();
		int teCount = tEnds.size();
		int rCount = rStarts.size();
		int reCount = rEnds.size();

		if ((mCount == tCount) && (mCount == teCount) && (mCount == rCount)
			&& (mCount == reCount))
		{
			for (int i = 0; i < mCount; ++i)
			{
				NUnionChangedStruct change;
				change.modelId = models.at(i)->getObjectId();
				change.posActive = true;
				change.posChange.start = tStarts.at(i);
				change.posChange.end = tEnds.at(i);
				change.rotateActive = true;
				change.rotateChange.start = rStarts.at(i);
				change.rotateChange.end = rEnds.at(i);
				changes.push_back(change);
			}
		}
		mixUnions(changes, reversible);
	}

	void ModelSpace::mixUnions(const QList<NUnionChangedStruct>& changes, bool reversible)
	{
		if (changes.size() == 0)
			return;

		if (reversible)
		{
			ModelSpaceUndo* stack = getKernel()->modelSpaceUndo();
			stack->mix(changes);
			return;
		}

		_mixUnions(changes, true);
	}

	void ModelSpace::_mixUnions(const QList<NUnionChangedStruct>& structs, bool redo)
	{
		QList<ModelN*> modelList;

		for (const NUnionChangedStruct& changeStruct : structs)
		{
			ModelN* model = getModelNByObjectId(changeStruct.modelId);
			modelList.append(model);

			if (changeStruct.rotateActive)
			{
				model->updateDisplayRotation(redo);

				QQuaternion q = redo ? changeStruct.rotateChange.end : changeStruct.rotateChange.start;
				_setModelRotation(model, q, false);
				model->resetNestRotation();

			}

			if (changeStruct.scaleActive)
			{
				QVector3D scale = redo ? changeStruct.scaleChange.end : changeStruct.scaleChange.start;
				_setModelScale(model, scale, false);
			}

			if (changeStruct.posActive)
			{
				QVector3D pos = redo ? changeStruct.posChange.end : changeStruct.posChange.start;
				_setModelPosition(model, pos, false);
			}

			model->dirty();
			
		}

		updateSpaceNodeRender(modelList);
	}

	void ModelSpace::setModelPosition(ModelN* model, const QVector3D& end, bool update)
	{
		_setModelPosition(model, end, update);
	}

	void ModelSpace::setModelRotation(ModelN* model, const QQuaternion& end, bool update)
	{
		_setModelRotation(model, end, update);
	}

	void ModelSpace::setModelScale(ModelN* model, const QVector3D& end, bool update)
	{
		_setModelScale(model, end, update);
	}

	void ModelSpace::updateModel(ModelN* model)
	{
	}

	void ModelSpace::fillChangeStructs(const QList<ModelN*>& models, QList<NUnionChangedStruct>& changes, bool start)
	{
		const QList<ModelN*>& selections = models;
		if (start)
		{
			changes.clear();
			for (ModelN* model : selections)
			{
				NUnionChangedStruct change;
				change.posActive = true;
				change.scaleActive = true;
				change.rotateActive = true;
				change.modelId = model->getObjectId();
				change.posChange.start = model->localPosition();
				change.rotateChange.start = model->localQuaternion();
				change.scaleChange.start = model->localScale();
				changes.push_back(change);
			}
		}
		else
		{
			QList<NUnionChangedStruct> _changes;
			for (const NUnionChangedStruct& change : changes)
			{
				ModelN* m = getModelNByObjectId(change.modelId);

				if (selections.contains(m))
				{
					NUnionChangedStruct c = change;
					c.posChange.end = m->localPosition();
					c.rotateChange.end = m->localQuaternion();
					c.scaleChange.end = m->localScale();
					_changes.push_back(c);
				}
			}
			changes.swap(_changes);
		}
	}

	void ModelSpace::fillChangeStructs(const QList<ModelGroup*>& groups, QList<NUnionChangedStruct>& changes, bool start)
	{
		if (groups.isEmpty())
		{
			return;
		}

		if (start)
		{
			changes.clear();
			for (ModelGroup* group : groups)
			{
				NUnionChangedStruct change;
				change.posActive = true;
				change.scaleActive = true;
				change.rotateActive = true;
				change.colorActive = true;
				change.modelGroupId = group->getObjectId();
				change.posChange.start = group->localPosition();
				change.rotateChange.start = group->localQuaternion();
				change.scaleChange.start = group->localScale();
				change.colorChange.start = group->defaultColorIndex();
				changes.push_back(change);
			}
		}
		else
		{
			QList<NUnionChangedStruct> _changes;
			for (const NUnionChangedStruct& change : changes)
			{
				ModelGroup* m = getModelGroupByObjectId(change.modelGroupId);
				if (groups.contains(m))
				{
					NUnionChangedStruct c = change;
					c.posChange.end = m->localPosition();
					c.rotateChange.end = m->localQuaternion();
					c.scaleChange.end = m->localScale();
					
					int color = m->defaultColorIndex();
					if (color == c.colorChange.start)
						c.colorActive = false;
					else 
						c.colorChange.end = color;

					_changes.push_back(c);
				}
			}
			changes.swap(_changes);
		}
	}

	void ModelSpace::resetAllModelsAndGroups(bool reversible)
	{
		QList<ModelN*> models = modelns();
		if (reversible)
			beginNodeSnap(m_sceneModelGroups, models);

		for (ModelGroup* model_group : m_sceneModelGroups)
			model_group->resetMatrix();

		for (ModelN* model : models)
			model->resetMatrix();

		endNodeSnap();
	}

	void ModelSpace::mergePosition(bool reversible)
	{
		QList<ModelGroup*> groups = m_model_selector->selectedGroups();
		mergePosition(groups, m_baseBoundingBox, reversible);
	}

	void ModelSpace::mergePosition(const QList<ModelGroup*>& modelGroups, const trimesh::dbox3& base, bool reversible)
	{
		if (modelGroups.size() == 0)
			return;

		QList<ModelN*> models;
		for (ModelGroup* group : modelGroups)
			models << group->models();

		if (reversible)
			beginNodeSnap(modelGroups, models);

		for (ModelGroup* group : modelGroups)
			group->resetToIdentity();

		for (ModelN* model : models)
		{
			cxkernel::MeshDataPtr d = model->data();
			if (d)
				model->setMatrix(trimesh::xform::trans(- d->offset));
		}

		trimesh::dbox3 b;
		for (ModelGroup* group : modelGroups)
			b += group->globalBoundingBox();

		trimesh::dvec3 offset = base.center() - b.center();
		offset.z = base.min.z - b.min.z;

		for (ModelGroup* group : modelGroups)
			group->applyMatrix(trimesh::xform::trans(offset));

		if (reversible)
			endNodeSnap();
	}

	void ModelSpace::bottomModel(ModelN* model, bool reversible)
	{
		if (!model)
			return;

		QList<ModelN*> models;
		models << model;
		bottomModels(models, reversible);
	}

	void ModelSpace::bottomModels(const QList<ModelN*>& models, bool reversible)
	{
		//how ? 
	}

	void ModelSpace::bottomModelGroup(ModelGroup* group, bool reversible)
	{
		if (!group)
			return;

		QList<ModelGroup*> groups;
		groups << group;
		bottomModelGroups(groups, reversible);
	}

	void ModelSpace::bottomModelGroups(const QList<ModelGroup*>& groups, bool reversible)
	{
		if (groups.size() == 0)
			return;

		if(reversible)
			beginNodeSnap(groups, QList<ModelN*>());

		for (ModelGroup* group : groups)
		{
			group->layerBottom();
		}

		if(reversible)
			endNodeSnap();
	}

	void ModelSpace::setModelGroupsMaxFaceBottom(const QList<ModelGroup*>& modelGroups)
	{
		beginNodeSnap(modelGroups, QList<ModelN*>());

		for (ModelGroup* group : modelGroups)
		{
			cxkernel::MeshDataPtr meshDataPtr = group->meshDataWithHullFaces();

			QList<int> maxIndexes;
			float maxArea = 0;
			for (int i = 0; i < meshDataPtr->faces.size(); ++i)
			{
				cxkernel::KernelHullFace face = meshDataPtr->faces[i];
				if (maxArea < face.hullarea)
				{
					if (abs(face.hullarea - maxArea) < 0.01)
					{	/*  equal */
						maxIndexes << i;
					}
					else
					{
						maxArea = face.hullarea;
						maxIndexes.clear();
						maxIndexes << i;
					}
				}
				else if (abs(face.hullarea - maxArea) < 0.01) // maxArea == face.hullarea
				{	/*  equal */
					maxIndexes << i;
				}
			}

			if (maxIndexes.isEmpty())
				continue;

			QVector3D downDir(0, 0, -1);
			float maxDot = -1000;
			int bestOption;
			for (int i : maxIndexes)
			{
				cxkernel::KernelHullFace face = meshDataPtr->faces[i];
				QVector3D qvec(face.normal[0], face.normal[1], face.normal[2]);
 				float dot = QVector3D::dotProduct(downDir, qvec);
				if (maxDot < dot)
				{
					maxDot = dot;
					bestOption = i;
				}
			}

			//if (!bestOptions.isEmpty())
			//{
			//	int nearsetOption;
			//	float minZ = 99999;
			//	for (int optionIndex : bestOptions)
			//	{
			//		cxkernel::KernelHullFace face = meshDataPtr->faces[optionIndex];
			//		face.mesh->need_bbox();
			//		float z = face.mesh->bbox.min.z;
			//		if (minZ > z)
			//		{
			//			nearsetOption = optionIndex;
			//			minZ = z;
			//		}
			//	}

				cxkernel::KernelHullFace face = meshDataPtr->faces[bestOption];
				trimesh::dvec3 normal = (trimesh::dvec3)face.normal;
				setModelGroupFaceBottom(group, normal);
			//}
		}

		for (ModelGroup* group : modelGroups)
		{
			group->layerBottom();
		}

		endNodeSnap();
	}

	void ModelSpace::setModelNsMaxFaceBottom(const QList<ModelN*>& models)
	{
		if (models.size() == 0)
			return;

		QList<ModelGroup*> groups;
		for (ModelN* model : models)
		{
			if (!groups.contains(model->getModelGroup()))
				groups.append(model->getModelGroup());
		}

		beginNodeSnap(groups, models);

		for (ModelN* model : models)
		{
			cxkernel::MeshDataPtr meshDataPtr = model->data();

			int maxIndex = -1;
			float maxArea = 0;
			for (int i = 0; i < meshDataPtr->faces.size(); ++i)
			{
				cxkernel::KernelHullFace face = meshDataPtr->faces[i];
				if (maxIndex == -1)
				{
					maxArea = face.hullarea;
					maxIndex = i;
				}
				else
				{
					if (maxArea < face.hullarea)
					{
						maxArea = face.hullarea;
						maxIndex = i;
					}
				}
			}

			if (maxIndex >= 0)
			{
				cxkernel::KernelHullFace face = meshDataPtr->faces[maxIndex];
				trimesh::dvec3 normal = (trimesh::dvec3)face.normal;

				setModelNFaceBottom(model, normal);
			}
		}

		for (ModelGroup* group : groups)
		{
			group->layerBottom();
		}

		endNodeSnap();
	}

	void ModelSpace::setModelGroupsFaceBottom(const QList<ModelGroup*>& modelGroups, const trimesh::dvec3& normal)
	{
		beginNodeSnap(modelGroups, QList<ModelN*>());

		for (ModelGroup* group : modelGroups)
		{
			setModelGroupFaceBottom(group, normal);
		}

		for (ModelGroup* group : modelGroups)
		{
			group->layerBottom();
		}

		endNodeSnap();
	}

	void ModelSpace::setModelNsFaceBottom(const QList<ModelN*>& models, const trimesh::dvec3& normal)
	{
		if (models.size() == 0)
			return;

		QList<ModelGroup*> groups;
		for (ModelN* model : models)
		{
			if (!groups.contains(model->getModelGroup()))
				groups.append(model->getModelGroup());
		}

		beginNodeSnap(groups, models);

		for (ModelN* model : models)
		{
			setModelNFaceBottom(model, normal);
		}

		for (ModelGroup* group : groups)
		{
			group->layerBottom();
		}

		endNodeSnap();
	}

	void ModelSpace::setModelNFaceBottom(ModelN* model, const trimesh::dvec3& normal)
	{
		if (!model)
			return;

		trimesh::dvec3 z = trimesh::dvec3(0.0, 0.0, -1.0);
		msbase::quaterniond q = msbase::quaterniond::rotationTo(normal, z);
		trimesh::xform xf = msbase::fromQuaterian(q);
		model->applyMatrix(xf);
	}

	void ModelSpace::setModelGroupFaceBottom(ModelGroup* model_group, const trimesh::dvec3& normal)
	{
		if (!model_group)
			return;

		trimesh::dvec3 c = model_group->globalBoundingBox().center();
		trimesh::dvec3 z = trimesh::dvec3(0.0, 0.0, -1.0);
		msbase::quaterniond q = msbase::quaterniond::rotationTo(normal, z);
		trimesh::xform xf = msbase::fromQuaterian(q);
		model_group->applyMatrix(trimesh::xform::trans(c) * xf * trimesh::xform::trans(-c));
	}

	void ModelSpace::_setModelPosition(ModelN* model, const QVector3D& end, bool update)
	{
		if (model->needInit())
			model->SetInitPosition(end);
		model->setLocalPosition(end, update);
		model->dirty();

	}

	void ModelSpace::_setModelRotation(ModelN* model, const QQuaternion& end, bool update)
	{
		model->setLocalQuaternion(end, update);
		model->dirty();
	}

	void ModelSpace::_setModelScale(ModelN* model, const QVector3D& end, bool update)
	{
		model->setLocalScale(end, update);
		model->dirty();
	}

	//void ModelSpace::_requestUpdate()
	//{
	//	setSpaceDirty(true);
	//	dirtyProjectSpace();
	//	requestVisUpdate(true);
	//}
}
