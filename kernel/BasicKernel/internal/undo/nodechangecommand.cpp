#include "nodechangecommand.h"
#include "interface/spaceinterface.h"
#include "interface/projectinterface.h"
#include "internal/project_cx3d/autosavejob.h"
namespace creative_kernel
{
	NodeChangeCommand::NodeChangeCommand(const QList<NodeChange>& changes, QObject* parent)
		:QObject(parent)
		, m_changes(changes)
	{
	}

	NodeChangeCommand::~NodeChangeCommand()
	{
	}

	void NodeChangeCommand::undo()
	{
		invoke(false);
	}

	void NodeChangeCommand::redo()
	{
		invoke(true);
	}

	void NodeChangeCommand::invoke(bool redo)
	{
		ModelSpace* space = getModelSpace();
		
		QList<ModelGroup*> needUpdateGroups;
		QList<ModelN*> needUpdateModels;
		for (NodeChange& nc : m_changes)
		{
			ModelGroup* group = space->getModelGroupByObjectId(nc.id);
			if (group)
			{
				group->setMatrix((redo ? nc.end_xf : nc.start_xf));
				needUpdateGroups.append(group);
				continue;
			}

			ModelN* model = space->getModelNByObjectId(nc.id);
			if (model)
			{
				model->setMatrix((redo ? nc.end_xf : nc.start_xf));
				needUpdateModels.append(model);
				continue;
			}
		}
		triggerAutoSave(needUpdateGroups,AutoSaveJob::MAT);
		space->updateSpaceNodeRender(needUpdateGroups);
		space->updateSpaceNodeRender(needUpdateModels);

		// fix bug:[ID1027707] [��ݼ�] ��X������תѡ��ģ����X�������򣬵���ģ�Ͳ�Ӧ�ó���2D����
		space->notifySpaceChange(false);
	}
}