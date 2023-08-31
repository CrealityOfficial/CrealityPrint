#ifndef OBJECTCHANGEDCOMMAND_H
#define OBJECTCHANGEDCOMMAND_H

#include <QtWidgets/QUndoCommand>
class LaserScene;

class ObjectChangedCommand : public QUndoCommand
{
public:
	ObjectChangedCommand(LaserScene* laser, QObject* obj, long long oldX, long long oldY, double oldWidth, double oldHeight, double oldRatation,
		long long newX, long long newY, double newWidth, double newHeight, double newRatation, bool undo);
	virtual ~ObjectChangedCommand();
protected:
	void undo() override;
	void redo() override;
private:
	LaserScene* m_laserScene;
	QObject* m_obj;
	bool m_isUndo;
	int64_t m_oldX;
	int64_t m_oldY;
	double m_oldWidth;
	double m_oldHeight;
	double m_oldRatation;
	int64_t m_newX;
	int64_t m_newY;
	double m_newWidth;
	double m_newHeight;
	double m_newRatation;
};


#endif // OBJECTCHANGEDCOMMAND_H
