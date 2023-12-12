#ifndef CREATIVE_KERNEL_MODELSPACEUNDO_1592743196335_H
#define CREATIVE_KERNEL_MODELSPACEUNDO_1592743196335_H
#include "basickernelexport.h"
#include <QtWidgets/QUndoStack>
#include "data/modeln.h"
#include "data/modelgroup.h"
#include "data/undochange.h"

namespace creative_kernel
{
	class BASIC_KERNEL_API ModelSpaceUndo : public QUndoStack
	{
		Q_OBJECT
	public:
		ModelSpaceUndo(QObject* parent = nullptr);
		virtual ~ModelSpaceUndo();

		void modifySpace(const QList<ModelN*>& models, const QList<ModelN*>& removeModels);

		void mix(const QList<NUnionChangedStruct>& mixChange);
		void mirror(const QList<NMirrorStruct>& mirrors);
		void push(QUndoCommand* cmd);
		time_t getActiveTime() const { return m_activeTime; }
	private:
		time_t m_activeTime{ 0 };
	};
}
#endif // CREATIVE_KERNEL_MODELSPACEUNDO_1592743196335_H