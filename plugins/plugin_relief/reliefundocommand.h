#ifndef CREATIVE_KERNEL_RELIEF_UNDO_COMMAND_1592790419297_H
#define CREATIVE_KERNEL_RELIEF_UNDO_COMMAND_1592790419297_H
#include "basickernelexport.h"
#include <QtWidgets/QUndoCommand>
#include "operation/fontmesh/fontmeshwrapper.h"

namespace creative_kernel
{
	class ModelN;
};

class ReliefOperateMode;

enum ReliefUndoConfigType
{
	UndoText = 0,
	UndoFont,
	UndoFontSize,
	UndoWordSpace,
	UndoLineSpace,
	UndoHeight,
	UndoDistance,
	UndoEmbossType,
	UndoBold,
	UndoItalic,
	UndoNone
}; 

struct ReliefUndoConfig
{
	int modelType;
	FontConfig config;

	ReliefUndoConfigType changedType{ UndoNone };
};

class ReliefUndoCommand : public QObject, public QUndoCommand
{
	Q_OBJECT
public:
	ReliefUndoCommand(QObject* parent = nullptr);
	virtual ~ReliefUndoCommand();

	void undo() override;
	void redo() override;

	void setOperater(ReliefOperateMode* operater);
	void setTarget(creative_kernel::ModelN* target);
	void setUndoConfig(ReliefUndoConfig oldConfig, ReliefUndoConfig newConfig);

protected:
	ReliefOperateMode* m_operater;

	int m_reliefId;
	ReliefUndoConfig m_old;
	ReliefUndoConfig m_new;

};

#endif // CREATIVE_KERNEL_RELIEF_UNDO_COMMAND_1592790419297_H