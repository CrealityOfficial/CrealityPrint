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
	public:
		ModelNSelector(QObject* parent = nullptr);
		virtual ~ModelNSelector();

		Q_INVOKABLE void sizeChanged();
		QList<ModelN*> selectionms();
		int selectedNum();
		Q_INVOKABLE QVector3D selectionmsSize();
	protected:
		void onSelectionsChanged() override;
		void selectChanged(qtuser_3d::Pickable* pickable) override;
	signals:
		void selectedNumChanged();
	};
}
#endif // CREATIVE_KERNEL_MODELNSELECTOR_1595659762117_H