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
	class ModelGroup;
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
	void setObjects(PaintOperateMode* colorOperateMode, creative_kernel::ModelGroup* modelGroups);
	void setData(const QList<std::vector<std::string>>& oldColorData, const QList<std::vector<std::string>>& newColorData);

protected:
	PaintOperateMode* m_colorOperateMode;
	// creative_kernel::ModelN* m_model;
	// QStringList m_serialName;
	int m_objectID;
	QList<std::vector<std::string>> m_oldData;
	QList<std::vector<std::string>> m_newData;
	bool m_needRedoOnFirst{ false };
};

#endif // CREATIVE_KERNEL_COLOR_UNDO_COMMAND_1592790419297_H