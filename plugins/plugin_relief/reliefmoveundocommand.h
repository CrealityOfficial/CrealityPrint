#ifndef CREATIVE_KERNEL_RELIEF_MOVE_UNDO_COMMAND_1592790419297_H
#define CREATIVE_KERNEL_RELIEF_MOVE_UNDO_COMMAND_1592790419297_H
#include "basickernelexport.h"
#include <QtWidgets/QUndoCommand>
#include "internal/alg/new_letter.h"
#include <QVector3D>

namespace creative_kernel
{
	class ModelN;
};

class ReliefOperateMode;

struct ReliefMoveConfig 
{
	int targetId { -1 };
	int faceId;
	trimesh::vec3 cross;
	int embossType;
};

class ReliefMoveUndoCommand : public QObject, public QUndoCommand
{
	Q_OBJECT
public:
	ReliefMoveUndoCommand(QObject* parent = nullptr);
	virtual ~ReliefMoveUndoCommand();

	void undo() override;
	void redo() override;

	void setOperater(ReliefOperateMode* operater);
	void needInit(bool need);
	void setRelief(creative_kernel::ModelN* relief);
	void setOldEmbossInfo(int targetId, int faceId, QVector3D location, int embossType);
	void setNewEmbossInfo(int targetId, int faceId, QVector3D location, int embossType);

	bool valid();

protected:
	ReliefOperateMode* m_operater;
	int m_reliefId;

	bool m_isOldEmboss { true }; 
	/* emboss */
	int m_oldTargetId;
	int m_oldFaceId;
	QVector3D m_oldCross;
	int m_oldEmbossType;

	/* new */ 
	/* emboss */
	int m_newTargetId;
	int m_newFaceId;
	QVector3D m_newCross;
	int m_newEmbossType;

};

#endif // CREATIVE_KERNEL_RELIEF_MOVE_UNDO_COMMAND_1592790419297_H