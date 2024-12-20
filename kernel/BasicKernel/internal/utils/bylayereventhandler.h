#ifndef CREATIVE_KERNEL_BY_LAYER_EVENT_HANDLER_202402211426_H
#define CREATIVE_KERNEL_BY_LAYER_EVENT_HANDLER_202402211426_H

#include <QObject>
#include "pmcollisioncheckhandler.h"

namespace creative_kernel {
	class ModelGroup;
	class Printer;

	class BASIC_KERNEL_API ByLayerEventHandler : public PMCollisionCheckHandler
	{
		Q_OBJECT
	public:
		ByLayerEventHandler(QObject* parent = nullptr);
		virtual ~ByLayerEventHandler();

		//bool processing() override;
		void clearCollisionFlags() override;
		QList<ModelGroup*> checkModelGroups(const QList<ModelGroup*>& groups) override;

	protected:
		virtual void onLeftMouseButtonPress(QMouseEvent* event) override {};
		virtual void onLeftMouseButtonRelease(QMouseEvent* event) override;
		virtual void onLeftMouseButtonMove(QMouseEvent* event) override {};
		virtual void onLeftMouseButtonClick(QMouseEvent* event) override {};

	private:

	public:
		static QList<ModelGroup*> collisionCheck(const QList<ModelGroup*>& models);
	};
}

#endif // !CREATIVE_KERNEL_BY_LAYER_EVENT_HANDLER_202402211426_H