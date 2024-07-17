#ifndef CREATIVE_KERNEL_MIRRORCOMMAND_1592874176854_H
#define CREATIVE_KERNEL_MIRRORCOMMAND_1592874176854_H
#include "basickernelexport.h"
#include <QtWidgets/QUndoCommand>
#include "external/data/modeln.h"
#include "data/undochange.h"

namespace creative_kernel
{
	class BASIC_KERNEL_API MirrorUndoCommand : public QObject, public QUndoCommand
	{
		Q_OBJECT
	public:
		MirrorUndoCommand(QObject* parent = nullptr);
		virtual ~MirrorUndoCommand();

		void setModels(const QList<ModelN*>& models);

		// 0: x, 1: y, 2: z
		void setMirrorMode(int mode);

	protected:
		void undo() override;
		void redo() override;
	private:
		void call(bool reverse);
		
	protected:
		QStringList m_serialNames;
		int m_mode;
	};
}
#endif // CREATIVE_KERNEL_MIRRORCOMMAND_1592874176854_H