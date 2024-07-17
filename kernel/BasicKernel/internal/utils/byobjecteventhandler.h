#ifndef CREATIVE_KERNEL_BY_OBJECT_EVENT_HANDLER_202401301711_H
#define CREATIVE_KERNEL_BY_OBJECT_EVENT_HANDLER_202401301711_H

#include <QtCore/QPointer>
#include "pmcollisioncheckhandler.h"

namespace creative_kernel {
	class ModelN;

	class BASIC_KERNEL_API ByObjectEventHandler : public PMCollisionCheckHandler
	{
		Q_OBJECT
	public:
		ByObjectEventHandler(QObject* parent = nullptr);
		virtual ~ByObjectEventHandler();

		//void initialize();
		bool processing() override;
		void clearCollisionFlags() override;
		QList<ModelN*> checkModels(const QList<ModelN*>& models) override;


		virtual void onLeftMouseButtonPress(QMouseEvent* event) override;
		virtual void onLeftMouseButtonRelease(QMouseEvent* event) override;
		virtual void onLeftMouseButtonMove(QMouseEvent* event) override {};
		virtual void onLeftMouseButtonClick(QMouseEvent* event) override {};
		
	protected:

		QList<ModelN*> collisionCheckAndUpdateState(const QList<ModelN*>& models);
		void clearCollisionFlags(const QList<QPointer<ModelN>>& modelPtrs);
		QList<ModelN*> _collisionCheck(const QList<QPointer<ModelN>>& modelPtrs);
		QList<ModelN*> _collisionCheck(const QList<ModelN*>& models);

		QList<ModelN*> _modelHeightCheckForSelectedPrinter();

		QList<ModelN*> _modelHeightCheck(const QList<ModelN*>& models);

		//for model render state
		void setStateNone(ModelN* m);
		void setStateCollide(ModelN* m);
		void setStateMoving(ModelN* m);
		void setStateCollide(const QList<ModelN*>& models);


		//
		void setStateTooTall(const QList<ModelN*>& models);

		void onExtruderClearanceHeightToLodChanged(float h);

		void updateHeightReferEntitySize();

	public:
		static QList<ModelN*> collisionCheck(const QList<ModelN*>& models);
		static QList<ModelN*> heightCheck(const QList<ModelN*>& models, float heightToRod);
	protected:
		QList<QPointer<ModelN>> m_modelPtrs;
		bool m_enable;
		bool m_processing;
	};

	float extruderClearanceHeightToRod();

}




#endif // !#ifndef CREATIVE_KERNEL_BY_OBJECT_EVENT_HANDLER_202401301711_H
