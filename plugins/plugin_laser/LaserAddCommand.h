#ifndef LASERADDCOMMAND_H
#define LASERADDCOMMAND_H

#include <QtWidgets/QUndoCommand>
class LaserScene;

class LaserAddCommand : public QUndoCommand
{
public:
	LaserAddCommand(LaserScene* laser, QObject* obj, QString sharpName, bool undo = false);
	virtual ~LaserAddCommand();
	void setIsRemove(bool isRemove);
protected:
	void undo() override;
	void redo() override;
private:
	LaserScene* m_laserScene;
	QObject* m_obj;
	bool m_isUndo;
	bool m_isRemove;
	QString m_sharpName;
};


#endif // LASERADDCOMMAND_H
