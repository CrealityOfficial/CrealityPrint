#ifndef CREATIVE_KERNEL_ADAPTIVE_UNDO_COMMAND_H
#define CREATIVE_KERNEL_ADAPTIVE_UNDO_COMMAND_H
#include <QtWidgets/QUndoCommand>
namespace creative_kernel
{
	class ModelN;
};

class AdaptiveLayerMode;
class AdaptiveUndoCommand : public QObject, public QUndoCommand
{
	Q_OBJECT
public:
	AdaptiveUndoCommand(QObject* parent = nullptr);
	virtual ~AdaptiveUndoCommand();

	void undo() override;
	void redo() override;

	void setObjects(AdaptiveLayerMode* operateMode, creative_kernel::ModelN* model);
	void setData(const std::vector<double>& oldColorData, const std::vector<double>& newColorData);

protected:
	AdaptiveLayerMode* m_operateMode;
	// creative_kernel::ModelN* m_model;
	QString m_serialName;
	std::vector<double> m_oldData;
	std::vector<double> m_newData;
	bool m_needRedoOnFirst{ false };
};

#endif // CREATIVE_KERNEL_ADAPTIVE_UNDO_COMMAND_H