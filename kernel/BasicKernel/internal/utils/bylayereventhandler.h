#ifndef CREATIVE_KERNEL_BY_LAYER_EVENT_HANDLER_202402211426_H
#define CREATIVE_KERNEL_BY_LAYER_EVENT_HANDLER_202402211426_H

#include <QObject>
#include "pmcollisioncheckhandler.h"

namespace creative_kernel {
	class ModelN;
	class Printer;

	class BASIC_KERNEL_API ByLayerEventHandler : public PMCollisionCheckHandler
	{
		Q_OBJECT
	public:
		ByLayerEventHandler(QObject* parent = nullptr);
		virtual ~ByLayerEventHandler();

		//bool processing() override;
		void clearCollisionFlags() override;
		QList<ModelN*> checkModels(const QList<ModelN*>& models) override;

	protected:
		virtual void onLeftMouseButtonPress(QMouseEvent* event) override {};
		virtual void onLeftMouseButtonRelease(QMouseEvent* event) override;
		virtual void onLeftMouseButtonMove(QMouseEvent* event) override {};
		virtual void onLeftMouseButtonClick(QMouseEvent* event) override {};

	private:
	};
}

#endif // !CREATIVE_KERNEL_BY_LAYER_EVENT_HANDLER_202402211426_H