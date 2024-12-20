#ifndef CREATIVE_KERNEL_RELIEF_ROTATE_UNDO_COMMAND_1592790419297_H
#define CREATIVE_KERNEL_RELIEF_ROTATE_UNDO_COMMAND_1592790419297_H
#include "basickernelexport.h"
#include <QtWidgets/QUndoCommand>

namespace creative_kernel
{
	class ModelN;
};

class ReliefOperateMode;

class ReliefRotateUndoCommand : public QObject, public QUndoCommand
{
	Q_OBJECT
public:
	ReliefRotateUndoCommand(QObject* parent = nullptr);
	virtual ~ReliefRotateUndoCommand();

	void undo() override;
	void redo() override;

	void setOperater(ReliefOperateMode* operater);
	void setRelief(creative_kernel::ModelN* relief);
	void setAngle(float oldAngle, float newAngle);

protected:
	ReliefOperateMode* m_operater;

	int m_reliefId;
	float m_oldAngle;
	float m_newAngle;

};

#endif // CREATIVE_KERNEL_RELIEF_ROTATE_UNDO_COMMAND_1592790419297_H