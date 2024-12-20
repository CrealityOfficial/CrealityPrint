#ifndef CREATIVE_KERNEL_MODELSPACEUNDO_1592743196335_H
#define CREATIVE_KERNEL_MODELSPACEUNDO_1592743196335_H
#include "basickernelexport.h"
#include <QtWidgets/QUndoStack>
#include "data/modeln.h"
#include "data/modelgroup.h"
#include "data/undochange.h"
#include "data/modelspace.h"

namespace creative_kernel
{
	class Printer;
	class BASIC_KERNEL_API ModelSpaceUndo : public QUndoStack
	{
		Q_OBJECT
	public:
		ModelSpaceUndo(QObject* parent = nullptr);
		virtual ~ModelSpaceUndo();

		void insertPrinter(int index, Printer* printer);
		void removePrinter(int index, Printer* printer);

		void changeMaterials(const QList<ModelN*>& models, int materialIndex);
		void mix(const QList<NUnionChangedStruct>& mixChange);
		void mixGroup(const QList<NUnionChangedStruct>& mixChange);

		void modifySpace(const SceneCreateData& scene_data);
		void modifyModelsMeshProperty(const QList<ModelNPropertyMeshUndo>& changes, bool reversible);
		void modifyNodes(const QList<NodeChange>& changes);

		void modifyWipeTower(const WipeTowerChange& data);
		

		void layoutChangeScene(const LayoutChangeInfo& changeInfo);
		void executeCommand(QUndoCommand* command);
	protected:
		void push(QUndoCommand* cmd);
	};
}
#endif // CREATIVE_KERNEL_MODELSPACEUNDO_1592743196335_H