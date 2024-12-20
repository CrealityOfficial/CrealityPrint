#include "_modelgroupinterface.h"
#include "data/modelgroup.h"
#include "data/modelspace.h"
#include "data/modelspaceundo.h"
#include "interface/spaceinterface.h"


namespace creative_kernel
{

	void _mixUnionsModelGroup(const QList<NUnionChangedStruct>& structs, bool redo)
	{
		QList<ModelGroup*> groupList;
		for (const NUnionChangedStruct& changeStruct : structs)
		{
			ModelGroup* modelGroup = getModelGroupByObjectId(changeStruct.modelGroupId);
			Q_ASSERT(modelGroup);

			groupList.append(modelGroup);
			if (changeStruct.rotateActive)
			{
				//modelGroup->updateDisplayRotation(redo);

				QQuaternion q = redo ? changeStruct.rotateChange.end : changeStruct.rotateChange.start;
				_setModelGroupRotation(modelGroup, q, false);
				//model->resetNestRotation();

			}

			if (changeStruct.scaleActive)
			{
				QVector3D scale = redo ? changeStruct.scaleChange.end : changeStruct.scaleChange.start;
				_setModelGroupScale(modelGroup, scale, true);
			}

			if (changeStruct.posActive)
			{
				QVector3D pos = redo ? changeStruct.posChange.end : changeStruct.posChange.start;
				_setModelGroupPosition(modelGroup, pos, true);
			}

			if (changeStruct.colorActive)
			{
				int color = redo ? changeStruct.colorChange.end : changeStruct.colorChange.start;
				_setModelGroupColor(modelGroup, color);
			}

			modelGroup->dirty();
		}

		updateSpaceNodeRender(groupList);
	}

	void _setModelGroupPosition(ModelGroup* modelGroup, const QVector3D& end, bool update)
	{
		//if (modelGroup->needInit())
		//	modelGroup->setInitPosition(end);
		modelGroup->setLocalPosition(end, update);
		modelGroup->dirty();
	}

	void _setModelGroupRotation(ModelGroup* modelGroup, const QQuaternion& end, bool update)
	{
		modelGroup->setLocalQuaternion(end, update);
		modelGroup->dirty();
	}

	void _setModelGroupScale(ModelGroup* modelGroup, const QVector3D& end, bool update)
	{
		modelGroup->setLocalScale(end, update);
		modelGroup->dirty();
	}

	void _setModelGroupColor(ModelGroup* modelGroup, const int& end)
	{
		modelGroup->setDefaultColorIndex(end);
		modelGroup->dirty();
	}

}
