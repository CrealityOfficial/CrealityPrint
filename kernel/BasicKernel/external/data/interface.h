#ifndef CREATIVE_KERNEL_INTERFACE_1595470868902_H
#define CREATIVE_KERNEL_INTERFACE_1595470868902_H
#include "data/header.h"
#include "data/kernelenum.h"
#include "qtuser3d/math/box3d.h"
#include "qtusercore/module/progressor.h"

namespace creative_kernel
{
	class UIVisualTracer
	{
	public:
		virtual ~UIVisualTracer() {};

		virtual void onThemeChanged(ThemeCategory category) = 0;
		virtual void onLanguageChanged(MultiLanguage language) = 0;
		//virtual void name() = 0;
	};

	class ModelN;
	class ModelGroup;
	class BASIC_KERNEL_API SpaceTracer
	{
	public:
		virtual ~SpaceTracer() {}

		virtual void onBoxChanged(const trimesh::dbox3& box);
		virtual void onSceneChanged(const trimesh::dbox3& box);

		virtual void onModelToAdded(ModelN* model) {}
		virtual void onModelToRemoved(ModelN* model) {}

		virtual void onModelAdded(ModelN* model) {}
		virtual void onModelRemoved(ModelN* model) {}

		virtual void onModelGroupAdded(ModelGroup* _model_group);
		virtual void onModelGroupRemoved(ModelGroup* _model_group);
		virtual void onModelGroupModified(ModelGroup* _model_group, const QList<ModelN*>& removes, const QList<ModelN*>& adds);

		virtual void onModelGroupNameChanged(ModelGroup* _model_group);
		virtual void onModelNameChanged(ModelN* model);
		virtual void onModelTypeChanged(ModelN* model);
	};

	class Printer;
	class BASIC_KERNEL_API ModelNSelectorTracer
	{
	public:
		virtual ~ModelNSelectorTracer() {}

		virtual void onSelectionsChanged();
		virtual void onSelectionsBoxChanged();
		virtual void onSelectionsObjectsChanged(const QList<Printer*>& printersOn, const QList<Printer*>& printersOff, const QList<ModelGroup*>& modelGroupsOn, const QList<ModelGroup*>& modelGroupsOff,
			const QList<ModelN*>& modelsOn, const QList<ModelN*>& modelsOff);
	};

	class Printer;
	class PrinterMangagerTracer
	{
	public:
		virtual ~PrinterMangagerTracer() {}

		virtual void willSettingPrinter(Printer* p) {};
		virtual void willAutolayoutForPrinter(Printer* p) {};
		virtual void willPickBottomForPrinter(Printer* p) {};
		virtual void willEditNameForPrinter(Printer* p, QString from) {};

		virtual void nameChanged(Printer* p, QString newName);
		virtual void printerIndexChanged(Printer* p, int newIndex);
		
		virtual void printersCountChanged(int count);
		virtual void modelGroupsOutOfPrinterChange(const QList<ModelGroup*>& list);
		virtual void didAddPrinter(Printer* p);
		virtual void didDeletePrinter(Printer* p);
		virtual void didSelectPrinter(Printer* p);

		virtual void printerNameChanged(Printer* p, QString newName);
		virtual void printerModelGroupsInsideChange(Printer* p, const QList<ModelGroup*>& list);
	};

	class KernelPhase
	{
	public:
		virtual ~KernelPhase() {}

		virtual void onStartPhase() = 0;
		virtual void onStopPhase() = 0;
	};

	class CloseHook
	{
	public:
		virtual ~CloseHook() {}

		virtual void onWindowClose() = 0;
	};
}
#endif // CREATIVE_KERNEL_INTERFACE_1595470868902_H