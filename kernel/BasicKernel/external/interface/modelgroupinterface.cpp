#include "modelgroupinterface.h"
#include "internal/data/_modelgroupinterface.h"
#include "external/data/modeln.h"
#include "external/data/modelgroup.h"
#include "data/modelspaceundo.h"
#include "kernel/kernel.h"
#include "external/data/modelspace.h"
#include "interface/spaceinterface.h"
#include "interface/selectorinterface.h"
#include "external/kernel/modelnselector.h"

namespace creative_kernel
{
	void mixUnionsModelGroup(const QList<NUnionChangedStruct>& changes, bool reversible)
	{
		if (changes.size() == 0)
			return;

		if (reversible)
		{
			ModelSpaceUndo* stack = getKernel()->modelSpaceUndo();
			stack->mixGroup(changes);
			return;
		}

		_mixUnionsModelGroup(changes, true);
	}

	void moveModelGroup(ModelGroup* group, const QVector3D& start, const QVector3D& end, bool reversible)
	{
		if (group == nullptr)
			return;

		QList<NUnionChangedStruct> changes;

		NUnionChangedStruct change;
		change.modelGroupId = group->getObjectId();
		change.posActive = true;
		change.posChange.start = start;
		change.posChange.end = end;
		changes.push_back(change);
	
		mixUnionsModelGroup(changes, reversible);
	}

	void moveModelGroups(const QList<ModelGroup*>& modelGroups, const QList<QVector3D>& starts, const QList<QVector3D>& ends, bool reversible)
	{
		QList<NUnionChangedStruct> changes;

		int mCount = modelGroups.size();
		int msCount = starts.size();
		int eCount = ends.size();

		if ((mCount == msCount) && (mCount == eCount))
		{
			for (int i = 0; i < mCount; ++i)
			{
				NUnionChangedStruct change;
				change.modelGroupId = modelGroups.at(i)->getObjectId();
				change.posActive = true;
				change.posChange.start = starts.at(i);
				change.posChange.end = ends.at(i);
				changes.push_back(change);
			}
		}

		mixUnionsModelGroup(changes, reversible);
	}


	BASIC_KERNEL_API void rotateModelGroups(const QList<int>& groupIds, const QList<QQuaternion>& rStarts, const QList<QQuaternion>& rEnds, bool reversible)
	{
		QList<NUnionChangedStruct> changes;
		int mCount = groupIds.size();
		int rCount = rStarts.size();
		int reCount = rEnds.size();

		if ((mCount == rCount) && (mCount == reCount))
		{
			for (int i = 0; i < mCount; ++i)
			{
				NUnionChangedStruct change;
				change.modelGroupId = groupIds.at(i);
				change.rotateActive = true;
				change.rotateChange.start = rStarts.at(i);
				change.rotateChange.end = rEnds.at(i);
				changes.push_back(change);
			}
		}
		mixUnionsModelGroup(changes, reversible);
	}

	void mixTRModelGroups(const QList<int>& groupIds, const QList<QVector3D>& tStarts, const QList<QVector3D>& tEnds,
		const QList<QQuaternion>& rStarts, const QList<QQuaternion>& rEnds, bool reversible)
	{
		QList<NUnionChangedStruct> changes;

		int mCount = groupIds.size();
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
				change.modelGroupId = groupIds.at(i);
				change.posActive = true;
				change.posChange.start = tStarts.at(i);
				change.posChange.end = tEnds.at(i);
				change.rotateActive = true;
				change.rotateChange.start = rStarts.at(i);
				change.rotateChange.end = rEnds.at(i);
				changes.push_back(change);
			}
		}
		mixUnionsModelGroup(changes, reversible);
	}


	void scaleModelGroups(const QList<int>& groupIds, const QList<QVector3D>& sStarts, const QList<QVector3D>& sEnds, bool reversible)
	{
		QList<NUnionChangedStruct> changes;

		int mCount = groupIds.size();
		int sCount = sStarts.size();
		int seCount = sEnds.size();
		if ((mCount == sCount) && (mCount == seCount))
		{
			for (int i = 0; i < mCount; ++i)
			{
				NUnionChangedStruct change;
				change.modelGroupId = groupIds.at(i);
				change.scaleActive = true;
				change.scaleChange.start = sStarts.at(i);
				change.scaleChange.end = sEnds.at(i);
				changes.push_back(change);
			}
		}
		mixUnionsModelGroup(changes, reversible);
	}

	void mixTSModelGroups(const QList<int>& groupIds, const QList<QVector3D>& tStarts, const QList<QVector3D>& tEnds,
		const QList<QVector3D>& sStarts, const QList<QVector3D>& sEnds, bool reversible)
	{
		QList<NUnionChangedStruct> changes;

		int mCount = groupIds.size();
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
				change.modelGroupId = groupIds.at(i);
				change.posActive = true;
				change.posChange.start = tStarts.at(i);
				change.posChange.end = tEnds.at(i);
				change.scaleActive = true;
				change.scaleChange.start = sStarts.at(i);
				change.scaleChange.end = sEnds.at(i);
				changes.push_back(change);
			}
		}
		mixUnionsModelGroup(changes, reversible);
	}

	void setModelGroupsColor(const QList<ModelGroup*>& modelGroups, int colorIndex, bool reversible)
	{
		if (reversible)
		{
			QList<creative_kernel::NUnionChangedStruct> changes;
			for (int i = 0, count = modelGroups.count(); i < count; ++i)
			{
				creative_kernel::NUnionChangedStruct cs;
				changes << cs;
			}

			fillChangeStructs(modelGroups, changes, true);
			for (ModelGroup* group : modelGroups)
			{
				group->setDefaultColorIndex(colorIndex);
			}
			fillChangeStructs(modelGroups, changes, false);
			mixUnionsModelGroup(changes, true);
		}
		else 
		{
			for (ModelGroup* group : modelGroups)
			{
				group->setDefaultColorIndex(colorIndex);
			}
		}
	}

}
