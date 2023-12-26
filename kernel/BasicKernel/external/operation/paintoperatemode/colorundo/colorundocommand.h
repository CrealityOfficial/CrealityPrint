#ifndef CREATIVE_KERNEL_COLOR_UNDO_COMMAND_1592790419297_H
#define CREATIVE_KERNEL_COLOR_UNDO_COMMAND_1592790419297_H
#include "basickernelexport.h"
#include <QtWidgets/QUndoCommand>

namespace spread
{
	class MeshSpreadWrapper;
	struct SceneData;
};

namespace creative_kernel
{
	class ModelN;
};

class PaintOperateMode;

class ColorUndoCommand : public QObject, public QUndoCommand
{
	Q_OBJECT
public:
	ColorUndoCommand(QObject* parent = nullptr);
	virtual ~ColorUndoCommand();

	void undo() override;
	void redo() override;

	void setObjects(PaintOperateMode* colorOperateMode, creative_kernel::ModelN* model);
	void setData(const std::vector<std::string>& oldColorData, const std::vector<std::string>& newColorData);

protected:
	PaintOperateMode* m_colorOperateMode;
	creative_kernel::ModelN* m_model;
	std::vector<std::string> m_oldData;
	std::vector<std::string> m_newData;
	bool m_needRedoOnFirst{ false };
};

#endif // CREATIVE_KERNEL_COLOR_UNDO_COMMAND_1592790419297_H