#ifndef CREATIVE_KERNEL_LAYOUT_MODELS_MOVE_COMMAND_202404231440_H
#define CREATIVE_KERNEL_LAYOUT_MODELS_MOVE_COMMAND_202404231440_H
#include "basickernelexport.h"
#include "data/undochange.h"
#include <QtWidgets/QUndoCommand>

namespace creative_kernel
{
	struct History_Data
	{
		QMap<int64_t, trimesh::xform> objectInfo;
		int printersNum;
	};

	class LayoutModelsMoveCommand : public QObject, public QUndoCommand
	{
		Q_OBJECT

	public:
		LayoutModelsMoveCommand(const LayoutChangeInfo& changeInfo, QObject* parent = nullptr);
		virtual ~LayoutModelsMoveCommand();

	protected:
		void undo() override;
		void redo() override;

		void sceneSnapshot(History_Data& historyData);
		void recoverSnapShot(const History_Data& historyData);

	protected:
		bool m_firstRedo;
		History_Data m_redoHistoryData;
		History_Data m_undoHistoryData;
	};
}
#endif // CREATIVE_KERNEL_LAYOUT_MODELS_MOVE_COMMAND_202404231440_H