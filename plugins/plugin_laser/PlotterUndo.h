#ifndef PLOTTERUNDO_H
#define PLOTTERUNDO_H

#include <QtWidgets/QUndoStack>

class LaserScene;
class PlotterUndo : public QUndoStack
{
	Q_OBJECT
public:
	PlotterUndo(QObject* parent = nullptr);
	virtual ~PlotterUndo();

	void addDragSharp(LaserScene* scene, QObject* obj, QString sharpName, bool isUndo, bool isRemove = false);
	void addObjectChanged(LaserScene* scene, QObject* obj, long long oldX, long long oldY, double oldWidth, double oldHeight, double oldRatation,
		long long newX, long long newY, double newWidth, double newHeight, double newRatation, bool isUndo);
};

#endif // PLOTTERUNDO_H
