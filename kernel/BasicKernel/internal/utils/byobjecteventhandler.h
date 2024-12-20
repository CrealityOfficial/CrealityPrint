#ifndef CREATIVE_KERNEL_BY_OBJECT_EVENT_HANDLER_202401301711_H
#define CREATIVE_KERNEL_BY_OBJECT_EVENT_HANDLER_202401301711_H

#include <QtCore/QPointer>
#include "pmcollisioncheckhandler.h"

namespace creative_kernel {
	class ModelGroup;

	class BASIC_KERNEL_API ByObjectEventHandler : public PMCollisionCheckHandler
	{
		Q_OBJECT
	public:
		ByObjectEventHandler(QObject* parent = nullptr);
		virtual ~ByObjectEventHandler();

		//void initialize();
		bool processing() override;
		void clearCollisionFlags() override;
		QList<ModelGroup*> checkModelGroups(const QList<ModelGroup*>& models) override;


		virtual void onLeftMouseButtonPress(QMouseEvent* event) override;
		virtual void onLeftMouseButtonRelease(QMouseEvent* event) override;
		virtual void onLeftMouseButtonMove(QMouseEvent* event) override {};
		virtual void onLeftMouseButtonClick(QMouseEvent* event) override {};
		
	protected:

		QList<ModelGroup*> collisionCheckAndUpdateState(const QList<ModelGroup*>& models);
		void clearCollisionFlags(const QList<QPointer<ModelGroup>>& modelPtrs);
		QList<ModelGroup*> _collisionCheck(const QList<QPointer<ModelGroup>>& modelPtrs);
		QList<ModelGroup*> _collisionCheck(const QList<ModelGroup*>& models);

		QList<ModelGroup*> _modelHeightCheckForSelectedPrinter();

		QList<ModelGroup*> _modelHeightCheck(const QList<ModelGroup*>& models);

		//for model render state
		void setStateNone(ModelGroup* m);
		void setStateCollide(ModelGroup* m);
		void setStateMoving(ModelGroup* m);
		void setStateCollide(const QList<ModelGroup*>& models);


		//
		void setStateTooTall(const QList<ModelGroup*>& models);

		void onExtruderClearanceHeightToLodChanged(float h);

		void updateHeightReferEntitySize();

	public:
		static QList<ModelGroup*> collisionCheck(const QList<ModelGroup*>& models);
		static QList<ModelGroup*> heightCheck(const QList<ModelGroup*>& models, float heightToRod);
	protected:
		QList<QPointer<ModelGroup>> m_modelPtrs;
		bool m_enable;
		bool m_processing;
	};

	float extruderClearanceHeightToRod();

}




#endif // !#ifndef CREATIVE_KERNEL_BY_OBJECT_EVENT_HANDLER_202401301711_H
