#include "interface.h"

namespace creative_kernel
{
	void SpaceTracer::onBoxChanged(const trimesh::dbox3& box)
	{

	}

	void SpaceTracer::onSceneChanged(const trimesh::dbox3& box)
	{

	}

	void SpaceTracer::onModelGroupAdded(ModelGroup* model_group)
	{

	}

	void SpaceTracer::onModelGroupRemoved(ModelGroup* model_group)
	{

	}

	void SpaceTracer::onModelGroupModified(ModelGroup* _model_group, const QList<ModelN*>& removes, const QList<ModelN*>& adds)
	{
	}

	void SpaceTracer::onModelGroupNameChanged(ModelGroup* _model_group)
	{

	}

	void SpaceTracer::onModelNameChanged(ModelN* model)
	{

	}

	void SpaceTracer::onModelTypeChanged(ModelN* model)
	{

	}

	void ModelNSelectorTracer::onSelectionsChanged()
	{

	}

	void ModelNSelectorTracer::onSelectionsBoxChanged()
	{

	}

	void ModelNSelectorTracer::onSelectionsObjectsChanged(const QList<Printer*>& printersOn, const QList<Printer*>& printersOff, const QList<ModelGroup*>& modelGroupsOn, const QList<ModelGroup*>& modelGroupsOff,
		const QList<ModelN*>& modelsOn, const QList<ModelN*>& modelsOff)
	{

	}

	void PrinterMangagerTracer::nameChanged(Printer* p, QString newName)
	{
	}

	void PrinterMangagerTracer::printerIndexChanged(Printer* p, int newIndex)
	{

	}

	void PrinterMangagerTracer::printersCountChanged(int count)
	{

	}

	void PrinterMangagerTracer::modelGroupsOutOfPrinterChange(const QList<ModelGroup*>& list)
	{

	}

	void PrinterMangagerTracer::didAddPrinter(Printer* p)
	{

	}

	void PrinterMangagerTracer::didDeletePrinter(Printer* p)
	{

	}

	void PrinterMangagerTracer::didSelectPrinter(Printer* p)
	{

	}

	void PrinterMangagerTracer::printerNameChanged(Printer* p, QString newName)
	{

	}

	void PrinterMangagerTracer::printerModelGroupsInsideChange(Printer* p, const QList<ModelGroup*>& list)
	{

	}
}
