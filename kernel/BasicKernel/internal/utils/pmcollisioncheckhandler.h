#ifndef PRINTER_MANAGER_COLLISION_CHECK_HANDLER_202402211600_H
#define PRINTER_MANAGER_COLLISION_CHECK_HANDLER_202402211600_H

#include <QObject>
#include "external/basickernelexport.h"
#include "qtuser3d/event/eventhandlers.h"

namespace creative_kernel {
	class ModelN;

	class BASIC_KERNEL_API PMCollisionCheckHandler : public QObject, public qtuser_3d::LeftMouseEventHandler
	{
		Q_OBJECT
	public:
		PMCollisionCheckHandler(QObject* parent = nullptr);
		virtual ~PMCollisionCheckHandler();

		virtual bool processing() { return false; };
		virtual QList<ModelN*> checkModels(const QList<ModelN*>& models) { return QList<ModelN*>(); };
		virtual void clearCollisionFlags() {};

	};
}

#endif // !PRINTER_MANAGER_COLLISION_CHECK_HANDLER_202402211600_H
