#ifndef CREATIVE_KERNEL_MODELNSELECTOR_1595659762117_H
#define CREATIVE_KERNEL_MODELNSELECTOR_1595659762117_H
#include "basickernelexport.h"
#include "qtuser3d/module/selector.h"

namespace creative_kernel
{
	class ModelN;
	class BASIC_KERNEL_API ModelNSelector : public qtuser_3d::Selector
		, public qtuser_3d::SelectorTracer
	{
		Q_OBJECT
		Q_PROPERTY(int selectedNum READ selectedNum NOTIFY selectedNumChanged)
		Q_PROPERTY(QList<QObject*> selectionmObjects READ selectionmObjects)
		Q_PROPERTY(bool wipeTowerSelected READ wipeTowerSelected NOTIFY wipeTowerSelectedChanged)
	public:
		ModelNSelector(QObject* parent = nullptr);
		virtual ~ModelNSelector();

		Q_INVOKABLE void sizeChanged();
		QList<ModelN*> selectionms();
		int selectedNum();
		Q_INVOKABLE QVector3D selectionmsSize();

		bool wipeTowerSelected() { return m_wipeTowerSelected; }

		void selectRect(const QRect& rect, bool exclude = true) override;
		void selectGroup(qtuser_3d::Pickable* pickable) override;

	private:
		QList<QObject*> selectionmObjects();

	protected:
		void onSelectionsChanged() override;
		void selectChanged(qtuser_3d::Pickable* pickable) override;
	signals:
		void selectedNumChanged();
		void wipeTowerSelectedChanged();

	private:
		bool m_wipeTowerSelected { false };
	};
}
#endif // CREATIVE_KERNEL_MODELNSELECTOR_1595659762117_H