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
#include "cxkernel/interface/jobsinterface.h"

#include "job/nest2djob.h"

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

	void addModelLayout(ModelN* model)
	{
		if (currentMachineIsBelt())
		{
			ModelPositionInitializer::layoutBelt(model, nullptr);
			bottomModel(model);
			addModel(model, true);
			model->updateMatrix();
		}
		else
		{
			Nest2DJob* job = new Nest2DJob();
			job->setInsert(model);
			cxkernel::executeJob(qtuser_core::JobPtr(job), true);
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

	void modifySpace(const QList<ModelN*>& removes, const QList<ModelN*>& adds, bool reversible)
	{
		if ((removes.size() == 0 && adds.size() == 0))
			return;

		if (reversible)
		{
			ModelSpaceUndo* stack = getKernel()->modelSpaceUndo();
			stack->modifySpace(adds, removes);
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
	}

	time_t getModelActiveTime()
	{
		return getKernel()->modelSpaceUndo()->getActiveTime();
	}

	void replaceModelsMesh(const QList<MeshChange>& changes, bool reversible)
	{
		if (reversible)
		{
			ModelSpaceUndo* stack = getKernel()->modelSpaceUndo();
			stack->replaceModels(changes);
			return;
		}

		_replaceModelsMesh(changes);
	}

	void replaceModelsMesh(const QList<ModelN*>& models, const QList<TriMeshPtr>& meshes, const QList<QString>& names, bool reversible)
	{
		replaceModelsMesh(generateMeshChanges(models, meshes, names), reversible);
	}

	QList<MeshChange> generateMeshChanges(const QList<ModelN*>& models, const QList<TriMeshPtr>& meshes, const QList<QString>& names)
	{
		QList<MeshChange> changes;
		int mCount = models.size();
		int meCount = meshes.size();
		int nCount = names.size();
		if ((mCount == meCount) && (mCount == nCount))
		{
			for (int i = 0; i < mCount; ++i)
			{
				ModelN* model = models.at(i);
				MeshChange change;
				change.model = model;
				change.start = model->meshptr();
				change.startName = model->objectName();
				change.end = meshes.at(i);
				change.endName = names.at(i);
				changes.append(change);
			}
		}
		return changes;
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
				change.model = models.at(i);
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
				change.model = models.at(i);
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
				change.model = models.at(i);
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
				change.model = model;
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
				if (selections.contains(change.model))
				{
					NUnionChangedStruct c = change;
					c.posChange.end = change.model->localPosition();
					c.rotateChange.end = change.model->localQuaternion();
					c.scaleChange.end = change.model->localScale();
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
		mirrorModels(generateMirrorChanges(models, MirrorOperation::mo_x), reversible);
	}

	void mirrorY(const QList<ModelN*>& models, bool reversible)
	{
		mirrorModels(generateMirrorChanges(models, MirrorOperation::mo_y), reversible);
	}

	void mirrorZ(const QList<ModelN*>& models, bool reversible)
	{
		mirrorModels(generateMirrorChanges(models, MirrorOperation::mo_z), reversible);
	}

	void mirrorReset(const QList<ModelN*>& models, bool reversible)
	{
		mirrorModels(generateMirrorChanges(models, MirrorOperation::mo_reset), reversible);
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

	void mirrorModels(const QList<NMirrorStruct>& changes, bool reversible)
	{
		if (reversible)
		{
			ModelSpaceUndo* stack = getKernel()->modelSpaceUndo();
			stack->mirror(changes);
			return;
		}

		_mirrorModels(changes);
	}

	QList<NMirrorStruct> generateMirrorChanges(const QList<ModelN*>& models, MirrorOperation operation)
	{
		QList<NMirrorStruct> changes;
		for (ModelN* model : models)
		{
			NMirrorStruct mirrorStruct;
			mirrorStruct.model = model;
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
		QList<ModelN*> models = modelns();
		for (ModelN* model : models)
			model->setVisualMode(mode);
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
		for (ModelN* model : models)
			model->setNozzle(nozzle);
		_requestUpdate();
	}

	QList<QVector3D> getModelHorizontalContour(ModelN* model) {
		if (!model) {
			return {};
		}

		return model->qConvex(true);
	}
}
