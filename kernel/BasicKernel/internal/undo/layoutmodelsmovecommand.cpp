#include "layoutmodelsmovecommand.h"
#include "data/modeln.h"
#include "interface/printerinterface.h"
#include "interface/modelgroupinterface.h"
#include "internal/multi_printer/printermanager.h"
#include "interface/layoutinterface.h"
#include "interface/spaceinterface.h"
#include "msbase/data/conv.h"

namespace creative_kernel
{
	LayoutModelsMoveCommand::LayoutModelsMoveCommand(const LayoutChangeInfo& changeInfo, QObject* parent)
		:m_firstRedo(true)
	{
		// (1) take scene snapShot for undo
		sceneSnapshot(m_redoHistoryData);

		QList<ModelGroup*> allMoveGroups;

		QList<int64_t> groupIds;
		QList<trimesh::dvec3> printer_offsets;

		// (2) check whether there are groups to move if resizePrinters
		checkGroupsToMove(changeInfo.printersCount, groupIds, printer_offsets);
		Q_ASSERT(groupIds.size() == printer_offsets.size());

		for (int i = 0; i < groupIds.size(); i++)
		{
			ModelGroup* mg = getModelGroupByObjectId(groupIds[i]);
			if (mg)
			{
				mg->setMatrix(trimesh::xform::trans(printer_offsets[i]) * mg->getMatrix());
				allMoveGroups.append(mg);
			}
		}

		// (3) resizePrinters if needed
		resizePrinters(changeInfo.printersCount);

		// (4) move groups according to layout algothyms
		for (int i = 0; i < changeInfo.moveModelGroupIds.size(); i++)
		{
			ModelGroup* mg = getModelGroupByObjectId(changeInfo.moveModelGroupIds[i]);
			if (mg)
			{
				QVector3D layPos = changeInfo.endPoses[i];

				trimesh::xform group_matrix = mg->getMatrix();
				trimesh::dvec3 offset(0.0, 0.0, 0.0);
				offset.x = (layPos.x());
				offset.y = (layPos.y());

				trimesh::xform xf = trimesh::xform::trans(offset) * group_matrix;

				mg->setMatrix(xf);

				allMoveGroups.append(mg);
			}
		}

		updateSpaceNodeRender(allMoveGroups);
		notifySpaceChange(false);

		// (5) check empty printers to remove, to do?
		//checkEmptyPrintersToRemove();

	}
	
	LayoutModelsMoveCommand::~LayoutModelsMoveCommand()
	{
	}

	void LayoutModelsMoveCommand::undo()
	{
		if (m_firstRedo)
		{
			// (1) take scene snap shot for redo
			sceneSnapshot(m_undoHistoryData);

			QList<ModelGroup*> allMoveGroups;

			QList<int64_t> groupIds;
			QList<trimesh::dvec3> printer_offsets;

			// (2) check whether there are groups to move if resizePrinters
			checkGroupsToMove(m_redoHistoryData.printersNum, groupIds, printer_offsets);
			Q_ASSERT(groupIds.size() == printer_offsets.size());

			for (int i = 0; i < groupIds.size(); i++)
			{
				ModelGroup* mg = getModelGroupByObjectId(groupIds[i]);
				if (mg)
				{
					mg->setMatrix(trimesh::xform::trans(printer_offsets[i]) * mg->getMatrix());
					allMoveGroups.append(mg);
				}
			}

		}

		// (3) recover snapShot according to redo historyData
		recoverSnapShot(m_redoHistoryData);

		m_firstRedo = false;

		notifySpaceChange(false);
	}

	void LayoutModelsMoveCommand::redo()
	{
		if (!m_firstRedo)
		{
			recoverSnapShot(m_undoHistoryData);
			notifySpaceChange(false);
		}
	}

	void LayoutModelsMoveCommand::sceneSnapshot(History_Data& historyData)
	{
		historyData.objectInfo.clear();
		historyData.printersNum = printersCount();

		QList<ModelGroup*> allGroups = modelGroups();
		for (ModelGroup* mg : allGroups)
		{
			historyData.objectInfo[mg->getObjectId()] = mg->getMatrix();
		}
	}

	void LayoutModelsMoveCommand::recoverSnapShot(const History_Data& historyData)
	{
		resizePrinters(historyData.printersNum);

		QList<ModelGroup*> groups;

		auto moit = historyData.objectInfo.begin();
		for (; moit != historyData.objectInfo.end(); moit++)
		{
			int64_t groupId = moit.key();
			ModelGroup* mg = getModelGroupByObjectId(groupId);
			if (mg)
			{
				mg->setMatrix(moit.value());
				groups.append(mg);
			}
		}

		updateSpaceNodeRender(groups);

	}

}