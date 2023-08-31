#include "PlotterUndo.h"
#include "LaserAddCommand.h"
#include "ObjectChangedCommand.h"

PlotterUndo::PlotterUndo(QObject* parent)
	:QUndoStack(parent)
{
}

PlotterUndo::~PlotterUndo()
{
}

void PlotterUndo::addDragSharp(LaserScene* scene, QObject* obj, QString sharpName, bool isUndo, bool isRemove)
{
	LaserAddCommand* command = new LaserAddCommand(scene, obj, sharpName, isUndo);
	command->setIsRemove(isRemove);
	push(command);
}

void PlotterUndo::addObjectChanged(LaserScene* scene, QObject* obj, long long oldX, long long oldY, double oldWidth, double oldHeight, double oldRatation, long long newX, long long newY, double newWidth, double newHeight, double newRatation, bool isUndo)
{
	ObjectChangedCommand* command = new ObjectChangedCommand(scene, obj, oldX, oldY, oldWidth, oldHeight, oldRatation,
		newX, newY, newWidth, newHeight, newRatation, true);
	push(command);
}

