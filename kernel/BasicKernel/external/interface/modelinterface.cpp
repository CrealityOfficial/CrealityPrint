#include "modelinterface.h"

#include <limits>

#include "internal/data/_modelinterface.h"

#include "kernel/kernel.h"

#include "data/modeln.h"
#include "data/modelgroup.h"
#include "data/modelspace.h"
#include "data/modelspaceundo.h"
#include "utils/modelpositioninitializer.h"

#include "interface/visualsceneinterface.h"
#include "interface/selectorinterface.h"
#include "interface/reuseableinterface.h"
#include "interface/spaceinterface.h"
#include "interface/uiinterface.h"
#include "interface/machineinterface.h"
#include "interface/layoutinterface.h"
#include "interface/printerinterface.h"

#include "cxkernel/interface/jobsinterface.h"
#include "internal/undo/modelserialcommand.h"

#include "qtuser3d/trimesh2/conv.h"
#include "internal/menu/submenurecentfiles.h"
#include "external/job/mirrorjob.h"
#include "cxkernel/interface/jobsinterface.h"
#include "external/kernel/reuseablecache.h"

namespace creative_kernel
{
	void openMeshFile()
	{
		getKernel()->openMeshFile();
	}

	void appendResizeModel(ModelN* model)
	{
		getModelSpace()->appendResizeModel(model);
	}
	void setMostRecentFile(QString filename)
	{
		SubMenuRecentFiles::getInstance()->setMostRecentFile(filename);
	}
	void addModelLayout(ModelN* model)
	{
		if (currentMachineIsBelt())
		{
			ModelPositionInitializer::layoutBelt(model, nullptr);
			bottomModel(model);
			addModel(model, true);
			model->updateMatrix();
			model->dirty();
		}
		else
		{
			QList<ModelN*> models;
			models << model;
			creative_kernel::layoutModels(models, currentPrinterIndex());
		}
	}

	void addModel(ModelN* model, bool reversible)
	{
		if (!model)
			return;

		QList<ModelN*> removes;
		QList<ModelN*> adds;
		adds << model;
		modifySpace(removes, adds, reversible);
	}

	void addModels(QList<ModelN*> models, bool reversible)
	{
		QList<ModelN*> removes;
		QList<ModelN*> adds;
		adds << models;
		modifySpace(removes, adds, reversible);
	}

	void modifySpace(const QList<ModelN*>& removes, const QList<ModelN*>& adds, bool reversible)
	{
		if ((removes.size() == 0 && adds.size() == 0))
			return;

		if (reversible)
		{
			ModelSpaceUndo* stack = getKernel()->modelSpaceUndo();
			//stack->modifySpace(adds, removes);
			QList<ModelN*> a = adds;
			QList<ModelN*> b = removes;
			stack->push(new ModelSerialCommand(b, a));
			return;
		}

		_modifySpace(removes, adds);
	}

	void removeAllModel(bool reversible)
	{
		QList<ModelN*> removes = modelns();
		QList<ModelN*> adds;

		modifySpace(removes, adds, reversible);
	}

	void removeSelectionModels(bool reversible)
	{
		QList<ModelN*> removes = selectionms();
		QList<ModelN*> adds;
		modifySpace(removes, adds, reversible);

		creative_kernel::checkModelCollision();
	}

	QList<ModelN*> replaceModelsMesh(const QList<ModelN*>& models, const QList<cxkernel::ModelNDataPtr>& datas, bool reversible)
	{
		int count = models.size();
		assert(count == datas.size());
		QList<ModelN*> adds;
		for (int i = 0; i < count; ++i)
			adds.append(createModelFromData(datas.at(i), models.at(i)));

		modifySpace(models, adds,reversible);

		return adds;
	}

	QList<ModelN*> getModelnsBySerialName(const QStringList& names)
	{
		return getModelSpace()->getModelnsBySerialName(names);
	}

	ModelN* getModelNBySerialName(const QString& name)
	{
		return getModelSpace()->getModelNBySerialName(name);
	}

	ModelN* getModelNByObjectName(const QString& objectName)
	{
		return getModelSpace()->getModelNByObjectName(objectName);
	}

	void rotateXModels(float angle,bool reversible)
	{
		QVector3D axis(1.0, 0.0, 0.0);
		if(reversible) axis = QVector3D(-1.0, 0.0, 0.0);
		auto selectedModels = selectionms();
		for (size_t i = 0; i < selectedModels.size(); i++)
		{
			QQuaternion oldQ = selectedModels[i]->localQuaternion();
			QQuaternion q = selectedModels[i]->rotateByStandardAngle(axis, angle);

			qtuser_3d::Box3D box = selectedModels[i]->globalSpaceBox();
			QVector3D zoffset = QVector3D(0.0f, 0.0f, -box.min.z());
			QVector3D localPosition = selectedModels[i]->localPosition();
			QVector3D newPosition = localPosition + zoffset;
			//selectedModels[i]->setLocalPosition(newPosition);
			mixTRModel(selectedModels[i], localPosition, newPosition, oldQ, q, true);
			// emit rotateChanged();
			if (axis.x() != 0 || axis.y() != 0)
				selectedModels[i]->resetNestRotation();
		}
	}

	void rotateYModels(float angle,bool reversible)
	{
		QVector3D axis(0.0, 1.0, 0.0);
		if(reversible) axis = QVector3D(0.0, -1.0, 0.0);
		auto selectedModels = selectionms();
		for (size_t i = 0; i < selectedModels.size(); i++)
		{
			QQuaternion oldQ = selectedModels[i]->localQuaternion();
			QQuaternion q = selectedModels[i]->rotateByStandardAngle(axis, angle);

			qtuser_3d::Box3D box = selectedModels[i]->globalSpaceBox();
			QVector3D zoffset = QVector3D(0.0f, 0.0f, -box.min.z());
			QVector3D localPosition = selectedModels[i]->localPosition();
			QVector3D newPosition = localPosition + zoffset;
			//selectedModels[i]->setLocalPosition(newPosition);
			mixTRModel(selectedModels[i], localPosition, newPosition, oldQ, q, true);
			// emit rotateChanged();
			if (axis.x() != 0 || axis.y() != 0)
				selectedModels[i]->resetNestRotation();
		}
	}

	void rotateZModels(float angle,bool reversible)
	{
		QVector3D axis(0.0, 0.0, 1.0);
		if(reversible) axis = QVector3D(0.0, 0.0, -1.0);
		auto selectedModels = selectionms();
		for (size_t i = 0; i < selectedModels.size(); i++)
		{
			QQuaternion oldQ = selectedModels[i]->localQuaternion();
			QQuaternion q = selectedModels[i]->rotateByStandardAngle(axis, angle);

			qtuser_3d::Box3D box = selectedModels[i]->globalSpaceBox();
			QVector3D zoffset = QVector3D(0.0f, 0.0f, -box.min.z());
			QVector3D localPosition = selectedModels[i]->localPosition();
			QVector3D newPosition = localPosition + zoffset;
			//selectedModels[i]->setLocalPosition(newPosition);
			mixTRModel(selectedModels[i], localPosition, newPosition, oldQ, q, true);
			// emit rotateChanged();
			if (axis.x() != 0 || axis.y() != 0)
				selectedModels[i]->resetNestRotation();
		}
	}

	void moveXModels(float distance,bool reversible)
	{
		QVector3D delta = QVector3D(distance, 0.0f, 0.0f);
		if(reversible) delta = QVector3D(-distance, 0.0f, 0.0f);
	    auto selectedModels = selectionms();
		for (size_t i = 0; i < selectedModels.size(); i++)
		{
			QVector3D localPosition = selectedModels.at(i)->localPosition();
			moveModel(selectedModels.at(i), localPosition, localPosition + delta);
		}
		requestVisUpdate(true);
	}

	void moveYModels(float distance,bool reversible)
	{
		QVector3D delta = QVector3D(0.0f, distance, 0.0f);
		if(reversible) delta = QVector3D(0.0f, -distance, 0.0f);
	    auto selectedModels = selectionms();
		for (size_t i = 0; i < selectedModels.size(); i++)
		{
			QVector3D localPosition = selectedModels.at(i)->localPosition();
			moveModel(selectedModels.at(i), localPosition, localPosition + delta);
		}
		requestVisUpdate(true);
	}

	void moveModel(ModelN* model, const QVector3D& start, const QVector3D& end, bool reversible)
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

	void moveModels(const QList<ModelN*>& models, const QList<QVector3D>& starts, const QList<QVector3D>& ends, bool reversible)
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
				change.serialName = models.at(i)->getSerialName();
				change.posActive = true;
				change.posChange.start = starts.at(i);
				change.posChange.end = ends.at(i);
				changes.push_back(change);
			}
		}
		mixUnions(changes, reversible);
	}

	void bottomModel(ModelN* model, bool reversible)
	{
		if (!model)
			return;

		QList<ModelN*> models;
		models << model;
		bottomModels(models, reversible);
	}

	void bottomModels(const QList<ModelN*>& models, bool reversible)
	{
		QList<QVector3D> starts;
		QList<QVector3D> ends;
		for (ModelN* model : models)
		{
			starts.push_back(model->localPosition());
			ends.push_back(model->zeroLocation());
		}
		moveModels(models, starts, ends, reversible);
	}

	void bottomAllModels(bool reversible)
	{
		bottomModels(modelns(), reversible);
	}

	void bottomSelectionModels(bool reversible)
	{
		bottomModels(selectionms(), reversible);
	}

	void alignModels(const QList<ModelN*>& models, const QVector3D& position, bool reversible)
	{
		QList<QVector3D> starts;
		QList<QVector3D> ends;

		float lowestZ = 999999.0f;
		for (ModelN* model : models)
		{
			QVector3D loc = model->localPosition();
			starts.push_back(loc);

			if (model->localZ() < lowestZ)
				lowestZ = model->localZ();
		}

		for (ModelN* model : models)
		{
			QVector3D loc = model->localPosition();
			qtuser_3d::Box3D box = model->calculateGlobalSpaceBox();
			QVector3D offset = position - box.center();
			offset.setZ(-box.min.z());

			QVector3D nLoc = loc + offset;
			nLoc.setZ(nLoc.z() + model->localZ() - lowestZ);
			ends.push_back(nLoc);
		}
		moveModels(models, starts, ends, reversible);
	}

	void alignAllModels(const QVector3D& position, bool reversible)
	{
		QList<ModelN*> models = modelns();
		alignModels(models, position, reversible);
	}

	void alignAllModels2BaseCenter(bool reversible)
	{
		qtuser_3d::Box3D box = sceneBoundingBox();
		qtuser_3d::Box3D base = baseBoundingBox();

		QVector3D position = base.center();
		position.setZ(box.center().z());

		alignAllModels(position, reversible);
	}

	void mergePosition(bool reversible)
	{
		QList<ModelN*> models = modelns();
		mergePosition(models, reversible);
	}

	void mergePosition(const QList<ModelN*>& models, bool reversible)
	{
		if (models.size() <= 0)
			return;

		//reset
		QList<NUnionChangedStruct> changes;
		int mCount = models.size();
		for (int i = 0; i < mCount; ++i)
		{
			cxkernel::ModelNDataPtr data = models.at(i)->data();
			NUnionChangedStruct change;
			change.serialName = models.at(i)->getSerialName();
			change.posActive = true;
			change.posChange.start = models.at(i)->localPosition();
			change.rotateActive = true;
			change.rotateChange.start = models.at(i)->localQuaternion();
			change.rotateChange.end = QQuaternion();
			change.scaleActive = true;
			change.scaleChange.start = models.at(i)->localScale();
			change.scaleChange.end = QVector3D(1.0f, 1.0f, 1.0f);

			models.at(i)->resetLocalQuaternion();
			models.at(i)->resetLocalScale();
			models.at(i)->setLocalPosition(- qtuser_3d::vec2qvector(data->offset));
			changes.push_back(change);
		}
		//
		qtuser_3d::Box3D base = baseBoundingBox();
		qtuser_3d::Box3D box;
		for (int i = 0; i < mCount; ++i)
		{
			models.at(i)->updateMatrix();
			box += models.at(i)->calculateGlobalSpaceBox();
		}
		QVector3D offset = base.center() - box.center();
		offset.setZ(base.min.z() - box.min.z());

		for (int i = 0; i < mCount; ++i)
		{
			V3Change& change = changes[i].posChange;
			change.end = models.at(i)->localPosition() + offset;
		}
		//
		mixUnions(changes, reversible);
	}
	
	void mergePosition(const QList<ModelN*>& models, const qtuser_3d::Box3D& base, bool reversible)
	{
		if (models.size() <= 0)
			return;

		//reset
		QList<NUnionChangedStruct> changes;
		int mCount = models.size();
		for (int i = 0; i < mCount; ++i)
		{
			cxkernel::ModelNDataPtr data = models.at(i)->data();
			NUnionChangedStruct change;
			change.serialName = models.at(i)->getSerialName();
			change.posActive = true;
			change.posChange.start = models.at(i)->localPosition();
			change.rotateActive = true;
			change.rotateChange.start = models.at(i)->localQuaternion();
			change.rotateChange.end = QQuaternion();
			change.scaleActive = true;
			change.scaleChange.start = models.at(i)->localScale();
			change.scaleChange.end = QVector3D(1.0f, 1.0f, 1.0f);

			models.at(i)->resetLocalQuaternion();
			models.at(i)->resetLocalScale();
			models.at(i)->setLocalPosition(- qtuser_3d::vec2qvector(data->offset));
			changes.push_back(change);
		}
		//
		qtuser_3d::Box3D box;
		for (int i = 0; i < mCount; ++i)
		{
			models.at(i)->updateMatrix();
			box += models.at(i)->calculateGlobalSpaceBox();
		}
		QVector3D offset = base.center() - box.center();
		offset.setZ(base.min.z() - box.min.z());

		for (int i = 0; i < mCount; ++i)
		{
			V3Change& change = changes[i].posChange;
			change.end = models.at(i)->localPosition() + offset;
		}
		//
		mixUnions(changes, reversible);
	}

	void mixTSModel(ModelN* model, const QVector3D& tstart, const QVector3D& tend,
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

	void mixTSModels(const QList<ModelN*>& models, const QList<QVector3D>& tStarts, const QList<QVector3D>& tEnds,
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
				change.serialName = models.at(i)->getSerialName();
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

	void mixTRModel(ModelN* model, const QVector3D& tstart, const QVector3D& tend,
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

	void mixTRModels(const QList<ModelN*>& models, const QList<QVector3D>& tStarts, const QList<QVector3D>& tEnds,
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
				change.serialName = models.at(i)->getSerialName();
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

	void mixUnions(const QList<NUnionChangedStruct>& changes, bool reversible)
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

	void resetAllModels(bool reversible)
	{
		ModelSpace* space = getModelSpace();
		QList<ModelN*> models = space->modelns();
		for (size_t i = 0; i < models.size(); i++)
		{
			QVector3D oldLocalPosition = models.at(i)->localPosition();
			QVector3D initPosition = models.at(i)->GetInitPosition();
			initPosition.setZ(0.0f);
			mirrorReset(models.at(i), true);
			setModelRotation(models.at(i), QQuaternion(), true);
			setModelScale(models.at(i), QVector3D(1.0f, 1.0f, 1.0f), true);
			moveModel(models.at(i), oldLocalPosition, models.at(i)->mapGlobal2Local(initPosition), true);
			updateModel(models.at(i));
		}
		checkModelRange();
		checkBedRange();
	}

	void setModelPosition(ModelN* model, const QVector3D& end, bool update)
	{
		_setModelPosition(model, end, update);
	}

	void setModelRotation(ModelN* model, const QQuaternion& end, bool update)
	{
		_setModelRotation(model, end, update);
	}

	void setModelScale(ModelN* model, const QVector3D& end, bool update)
	{
		_setModelScale(model, end, update);
	}

	void updateModel(ModelN* model)
	{
		_updateModel(model);
	}

	void rotateModelNormal2Bottom(ModelN* model, const QVector3D& normal)
	{
		QList<ModelN*> models;
		models << model;
		rotateModelsNormal2Bottom(models, normal);
		model->resetNestRotation();
	}

	void rotateModelsNormal2Bottom(const QList<ModelN*>& models, const QList<QVector3D>& normals)
	{
		QList<QVector3D> tStarts;
		QList<QVector3D> tEnds;
		QList<QQuaternion> rStarts;
		QList<QQuaternion> rEnds;
		for (int i = 0, count = models.size(); i < count; ++i)
		{
			ModelN* model = models[i];
			QVector3D normal;
			if (normals.size() <= i)
				normal = normals[0];
			else 
				normal = normals[i];

			QVector3D tStart = model->localPosition();
			QQuaternion rStart = model->localQuaternion();
			QVector3D tEnd;
			QQuaternion rEnd;
			model->rotateNormal2Bottom(normal, tEnd, rEnd);

			tStarts.append(tStart);
			tEnds.append(tEnd);
			rStarts.append(rStart);
			rEnds.append(rEnd);
		}
		mixTRModels(models, tStarts, tEnds, rStarts, rEnds, true);
	}

	void rotateModelsNormal2Bottom(const QList<ModelN*>& models, const QVector3D& normal)
	{
		QList<QVector3D> tStarts;
		QList<QVector3D> tEnds;
		QList<QQuaternion> rStarts;
		QList<QQuaternion> rEnds;
		for (ModelN* model : models)
		{
			QVector3D tStart = model->localPosition();
			QQuaternion rStart = model->localQuaternion();
			QVector3D tEnd;
			QQuaternion rEnd;
			model->rotateNormal2Bottom(normal, tEnd, rEnd);

			tStarts.append(tStart);
			tEnds.append(tEnd);
			rStarts.append(rStart);
			rEnds.append(rEnd);
		}
		mixTRModels(models, tStarts, tEnds, rStarts, rEnds, true);
	}

	void fillChangeStructs(QList<NUnionChangedStruct>& changes, bool start)
	{
		QList<ModelN*> selections = selectionms();
		if (start)
		{
			changes.clear();
			for (ModelN* model : selections)
			{
				NUnionChangedStruct change;
				change.posActive = true;
				change.scaleActive = true;
				change.rotateActive = true;
				change.serialName = model->getSerialName();
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
				ModelN* m = getModelNBySerialName(change.serialName);
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

	void mirrorX(ModelN* model, bool reversible)
	{
		if (!model)
			return;

		QList<ModelN*> models;
		models << model;
		mirrorX(models, reversible);
	}

	void mirrorY(ModelN* model, bool reversible)
	{
		if (!model)
			return;

		QList<ModelN*> models;
		models << model;
		mirrorY(models, reversible);
	}

	void mirrorZ(ModelN* model, bool reversible)
	{
		if (!model)
			return;

		QList<ModelN*> models;
		models << model;
		mirrorZ(models, reversible);
	}

	void mirrorReset(ModelN* model, bool reversible)
	{
		if (!model)
			return;

		QList<ModelN*> models;
		models << model;
		mirrorReset(models, reversible);
	}

	void mirrorX(const QList<ModelN*>& models, bool reversible)
	{
		mirrorModels(models, 0, reversible);
	}

	void mirrorY(const QList<ModelN*>& models, bool reversible)
	{
		mirrorModels(models, 1, reversible);
	}

	void mirrorZ(const QList<ModelN*>& models, bool reversible)
	{
		mirrorModels(models, 2, reversible);
	}

	void mirrorReset(const QList<ModelN*>& models, bool reversible)
	{
		//mirrorModels(generateMirrorChanges(models, MirrorOperation::mo_reset), reversible);
	}

	void mirrorXSelections(bool reversible)
	{
		mirrorX(selectionms(), reversible);
	}

	void mirrorYSelections(bool reversible)
	{
		mirrorY(selectionms(), reversible);
	}

	void mirrorZSelections(bool reversible)
	{
		mirrorZ(selectionms(), reversible);
	}

	void mirrorResetSelections(bool reversible)
	{
		mirrorReset(selectionms(), reversible);
	}

	void mirrorModels(const QList<ModelN*>& models, int mode, bool reversible)
	{
		if (reversible)
		{
			ModelSpaceUndo* stack = getKernel()->modelSpaceUndo();
			stack->mirror(models, mode);
			return;
		}

		MirrorJob* job = new MirrorJob(models, mode);
		cxkernel::executeJob(qtuser_core::JobPtr(job));
	}

	QList<NMirrorStruct> generateMirrorChanges(const QList<ModelN*>& models, MirrorOperation operation)
	{
		QList<NMirrorStruct> changes;
		for (ModelN* model : models)
		{
			NMirrorStruct mirrorStruct;
			mirrorStruct.serialName = model->getSerialName();
			mirrorStruct.operation = operation;
			mirrorStruct.end = QMatrix4x4();
			mirrorStruct.start = model->mirrorMatrix();
			changes.push_back(mirrorStruct);
		}
		return changes;
	}

	void cloneModels(const QList<ModelN*>& models, int count, bool reversible)
	{
		if ((count <= 0) || (count > 10))
			return;


		for (size_t i = 0; i < models.size(); i++)
		{
			ModelN* m = models.at(i);
			QString objectName = m->objectName();
			objectName.chop(4);
			creative_kernel::ModelN* model = new creative_kernel::ModelN();

			//for repair info
			model->setErrorEdges(m->getErrorEdges());
			model->setErrorNormals(m->getErrorNormals());
			model->setErrorHoles(m->getErrorHoles());
			model->setErrorIntersects(m->getErrorIntersects());

			model->setData(m->data());
			QString name = QString("%1#%2").arg(objectName).arg(i) + ".stl";
			model->setObjectName(name);
			model->setLocalScale(m->localScale());
			qtuser_3d::Box3D box = model->globalSpaceBox();
			QVector3D zoffset = QVector3D(0.0f, 0.0f, -box.min.z());
			QVector3D localPosition = model->localPosition();
			QVector3D newPosition = localPosition + zoffset;
			mixTSModel(model, model->localPosition(), newPosition, model->localScale(), m->localScale());
			addModel(model, true);
		}
	}

	void cloneSelectionModels(int count, bool reversible)
	{
		QList<ModelN*> selections = selectionms();
		cloneModels(selections, count, reversible);
	}

	void setModelVisualMode(ModelVisualMode mode)
	{
		getKernel()->reuseableCache()->setModelVisualMode(mode);
		_requestUpdate();
	}

	void setSelectionModelsNozzle(int nozzle)
	{
		QList<ModelN*> selections = selectionms();
		setModelsNozzle(selections, nozzle);
	}

	int getSelectionModelsNozzle()
	{
		QList<ModelN*> selections = selectionms();
		if (selections.size() > 1)
			return -1;
		if (selections.size() == 0)
			return -2;

		return selections[0]->nozzle();
	}

	void setModelsNozzle(const QList<ModelN*>& models, int nozzle)
	{
		for (ModelN* model : models) {
			model->setNozzle(nozzle);
			model->setDefaultColorIndex(nozzle);
		}
			
		_requestUpdate();
	}

	QList<QVector3D> getModelHorizontalContour(ModelN* model) {
		if (!model) {
			return {};
		}

		return model->qConvex(true);
	}

	ModelN* createModelFromData(cxkernel::ModelNDataPtr data, ModelN* replaceModel)
	{
		ModelN* model = new ModelN();
		model->setData(data);
		if (replaceModel)
		{
			model->setLocalPosition(replaceModel->localPosition());
			model->setLocalScale(replaceModel->localScale());
			model->setLocalQuaternion(replaceModel->localQuaternion());
			model->SetInitPosition(replaceModel->GetInitPosition());
		}

		return model;
	}
	
	bool applyMaterialColorsToMesh(trimesh::TriMesh* mesh, int& defaultColorIndex, QSet<int>& usedColorIndexs)
	{
		std::vector<trimesh::vec> colors = currentColors();
		int colorsCount = colors.size();
		if (colorsCount == 0)
			return false;

		int flagsCount = mesh->flags.size();
		if (flagsCount != mesh->faces.size())
		{
			flagsCount = mesh->faces.size();
			mesh->flags.resize(flagsCount);
		}

		/* 颜色索引说明
		 * mesh里的flag从1开始算，0代表默认颜色，此时使用defaultColorIndex。
		 */
		// bool hasColor = false;
		if (defaultColorIndex >= colorsCount || defaultColorIndex < 0)
			defaultColorIndex = 0;

		mesh->colors.resize(flagsCount);
		for (int i = 0; i < flagsCount; ++i) 
		{
			if (mesh->flags[i] > colorsCount)
				mesh->flags[i] = defaultColorIndex;	//索引值超出颜色数量，改为第一种颜色

			int flag = mesh->flags[i];
			// if (flag > 0)
			// 	hasColor = true;

			if (flag == 0)
			{
				usedColorIndexs.insert(defaultColorIndex);
				mesh->colors[i] = colors[defaultColorIndex];
			}
			else 
			{
				usedColorIndexs.insert(flag - 1);
				mesh->colors[i] = colors[flag - 1];	
			}
		}
		
		// if (!hasColor)
		// 	mesh->colors.clear();

		return true;
	}

	void applyMaterialColorsToAllModel()
	{
		QSet<ModelNRenderData*> cache;
		QList<ModelN*> ms = modelns();
		for (auto m : ms)
		{
			auto renderData = m->renderData();
			if (cache.contains(renderData.get()))
			{
				m->setRenderData(renderData);
			}
			else 
			{
				m->updateRender();
				cache.insert(renderData.get());
			}
		}
	}

	void setAllModelMaxFaceBottom()
	{
		setModelsMaxFaceBottom(modelns());
	}

	void setModelsMaxFaceBottom(QList<ModelN*>models)
	{
		QList<creative_kernel::ModelN*> operateModels;
		QList<QVector3D> normals;

		for (creative_kernel::ModelN* model : models)
		{
			model->data()->calculateFaces();

			const std::vector<cxkernel::KernelHullFace>& faces = model->data()->faces;
			if (faces.size() > 0)
			{
				const cxkernel::KernelHullFace& face = faces.at(0);

				trimesh::fxform xf = qtuser_3d::qMatrix2Xform(model->globalMatrix());
				trimesh::vec n = trimesh::norm_xf(xf) * face.normal;
				QVector3D normal = qtuser_3d::vec2qvector(trimesh::normalized(n));

				operateModels << model;
				normals << normal;
			}
		}	

		creative_kernel::rotateModelsNormal2Bottom(operateModels, normals);		
	}
}
