#include "printerinterface.h"
#include "internal/multi_printer/printermanager.h"
#include "kernel/kernel.h"
#include "external/kernel/reuseablecache.h"
#include "data/modelspaceundo.h"
#include "data/modelgroup.h"
#include "qtuser3d/trimesh2/conv.h"
#include "interface/layoutinterface.h"
#include "interface/selectorinterface.h"
#include "interface/spaceinterface.h"
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>

namespace creative_kernel
{
	PrinterMangager* getPrinterManager()
	{
		return getPrinterMangager();
	}

	int printersCount()
	{
		return getPrinterMangager()->printersCount();
	}

	Printer* getPrinter(int index)
	{
		return getPrinterMangager()->getPrinter(index);
	}

	Printer* currentPrinter()
	{
		return getPrinterMangager()->getSelectedPrinter();
	}

	qtuser_3d::Box3D currentPrinterGlobalBox()
	{
		Printer* printer = getPrinterMangager()->getSelectedPrinter();
		if (printer)
			return printer->globalBox();
		else 
			return qtuser_3d::Box3D();
	}

	QList<ModelN*> getPrinterModels(int index)
	{
		return getPrinterMangager()->getPrinterModels(index);
	}
	
	QList<ModelN*> getAllModelsOutOfPrinter()
	{
		return getPrinterMangager()->getAllModelsOutOfPrinter();
	}

	QList<int> getSlicedPrinterIndice()
	{
		return getPrinterMangager()->getSlicedPrinterIndice();
	}

	QList<QList<ModelGroup*>> getEachPrinterModelsGroups()
	{
		return getPrinterManager()->getEachPrinterModelsGroups();
	}

	QList<ModelGroup*> getAllPrinterModelsGroups()
	{
		return getPrinterManager()->getAllPrinterModelsGroups();
	}

	QByteArray getPrinterSerialData()
	{
		return getPrinterMangager()->getSerialData();
	}

	void createPrinterFromProject(const QByteArray& data)
	{
		QJsonDocument qdoc = QJsonDocument::fromJson(data);
		QJsonObject rootObj = qdoc.object();

		QJsonArray printerArray = rootObj.value("printers").toArray();
		int project_printer_count = printerArray.size();
		if (project_printer_count == 0)
		{
			return;
		}
		int current_printer_count = getPrinterMangager()->printersCount();
		if (current_printer_count > project_printer_count)
		{
			QMetaObject::invokeMethod(getPrinterMangager(), [current_printer_count, project_printer_count]() {
				getPrinterMangager()->removePrinters(current_printer_count - project_printer_count);
				}, Qt::ConnectionType::QueuedConnection);
		}
		for (int i = 0; i < project_printer_count; i++)
		{
			auto plateObj = printerArray.at(i).toObject();
			if (i < current_printer_count)
			{
				auto pm = getPrinterMangager();
				auto printer = pm->getPrinter(i);
				auto x = plateObj.value("x").toDouble();
				auto y = plateObj.value("y").toDouble();
				auto z = plateObj.value("z").toDouble();
				printer->setPosition(QVector3D(x, y, z));
				//QMetaObject::invokeMethod(getPrinterMangager(), [printer,x,y,z]() {
				//	printer->setPosition(QVector3D(x, y, z));
				//	}, Qt::ConnectionType::QueuedConnection);
			}
			else
			{
				QMetaObject::invokeMethod(getPrinterMangager(), []() {
					appendPrinter(false);
					}, Qt::ConnectionType::QueuedConnection);
			}
		}
	}

	bool appendPrinter(bool reversible)
	{
		auto pm = getPrinterMangager();
		if (pm->printerEnough())
		{
			return false;
		}

		int index = pm->printersCount();
		Printer* templatePrinter = pm->getPrinter(index - 1);
		Printer* newPrinter = new Printer(*templatePrinter);
		
		if (reversible)
		{
			ModelSpaceUndo* stack = getKernel()->modelSpaceUndo();
			stack ->insertPrinter(index, newPrinter);
		}
		else 
		{
			pm->insertPrinter(index, newPrinter);
		}

		return true;
	}

	void removePrinter(int index, bool reversible)
	{
		Printer* printer = getPrinter(index);
		
		if (reversible)
		{
			ModelSpaceUndo* stack = getKernel()->modelSpaceUndo();
			stack ->removePrinter(index, printer);
		}
		else 
		{
			getPrinterMangager()->deletePrinter(printer);
		}
	}

	bool resizePrinters(int num)
	{
		return getPrinterMangager()->resizePrinters(num);
	}

	void setPrinterVisibility(int printerIndex, bool visibility, bool exclude)
	{
		getPrinterMangager()->setPrinterVisibility(printerIndex, visibility, exclude);
	}

	void showPrinter(int printerIndex)
	{
		getPrinterMangager()->showPrinter(printerIndex);
	}

	void hidePrinter(int printerIndex)
	{
		getPrinterMangager()->hidePrinter(printerIndex);
	}

	bool hasModelsOnBorder(int index)
	{
		return getPrinterMangager()->hasModelsOnBorder(index);
	}

	Printer* getSelectedPrinter()
	{
		return getPrinterMangager()->getSelectedPrinter();
	}

	void selectPrinter(Printer* p)
	{
		getPrinterMangager()->selectPrinter(p);
	}

	void selectPrinter(int index, bool e)
	{
		getPrinterMangager()->selectPrinter(index, e);
	}

	void setPrinterPreviewMode(int printerIndex)
	{
		getPrinterMangager()->setPrinterPreviewMode(printerIndex);
	}
	
	void setPrinterEditMode(int printerIndex)
	{
		getPrinterMangager()->setPrinterEditMode(printerIndex);
	}

	void updateWipeTowers()
	{
		getPrinterMangager()->updateWipeTowers();
	}

	void resetPrinterNum()
	{
		int ncount = printersCount();
		if(ncount > 1)
			removePrinters(ncount - 1);
        Printer* printer = getPrinter(0);
        if (printer)
            printer->ResetSettings();
	}
	bool _checkModelInsideVisibleLayers(bool showAlert)
	{
		return getSelectedPrinter()->checkModelInsideVisibleLayers(showAlert);
	}
	QString getAllWipeTowerPositionX(QString separate)
	{
		return getPrinterMangager()->getAllWipeTowerPosition(0, separate);
	}

	QString getAllWipeTowerPositionY(QString separate)
	{
		return getPrinterMangager()->getAllWipeTowerPosition(1, separate);
	}

	void checkEmptyPrintersToRemove()
	{
		getPrinterManager()->checkEmptyPrintersToRemove();
	}

	QList<qtuser_3d::Box3D> estimateGlobalBoxBeenLayout(const qtuser_3d::Box3D& box, int num)
	{
		return PrinterMangager::estimateGlobalBoxBeenLayout(box, num);
	}

	int currentPrinterIndex()
	{
		Printer* selectedPrinter = getSelectedPrinter();
		if (!selectedPrinter)
			return 0;

		return selectedPrinter->index();
	}

	int getModelPrinterIndex(ModelN* model)
	{
		if (!model)
			return -1;

		int printerSize = printersCount();
		Printer* printerPtr = nullptr;
		for (int i = 0; i < printerSize; i++)
		{
			printerPtr = getPrinter(i);
			Q_ASSERT(printerPtr);
			qtuser_3d::Box3D printerBox = printerPtr->globalBox();
			printerBox.min.setZ(0.0f);
			printerBox.max.setZ(0.0f);

			qtuser_3d::Box3D modelBox = model->globalSpaceBox();
			modelBox.min.setZ(0.0f);
			modelBox.max.setZ(0.0f);

			if (printerBox.contains(modelBox))
				return printerPtr->index();
		}

		return -1;
	}

	void getAllWipeTowerOutline(QList< std::vector<trimesh::vec3> >& outPathList, QList<int>& outIndexList)
	{
		outPathList.clear();
		outIndexList.clear();

		int printerSize = printersCount();
		Printer* printerPtr = nullptr;
		for (int i = 0; i < printerSize; i++)
		{
			printerPtr = getPrinter(i);
			Q_ASSERT(printerPtr);
			outPathList.append(printerPtr->wipeTowerOutlinePath(true));
			outIndexList.append(printerPtr->index());
		}
	}

	std::vector<trimesh::vec3> getPrinterWipeTowerOutline(int printerIndex)
	{
		Q_ASSERT(printerIndex >= 0 && printerIndex < printersCount());

		Printer* printerPtr = getPrinterMangager()->getPrinter(printerIndex);
		Q_ASSERT(printerPtr);

		return printerPtr->wipeTowerOutlinePath(true);

	}

	QList< QPair<int, int> > getAllLockedPrinterModelGroupIds()
	{
		QList< QPair<int, int> > results;

		auto pm = getPrinterMangager();
		int count = pm->printersCount();

		for (size_t i = 0; i < count; i++)
		{
			Printer* prt = pm->getPrinter(i);
			if (prt && prt->lock())
			{
				QList<ModelGroup*> modelGroups = prt->modelGroupsInside();

				for (ModelGroup* mg : modelGroups)
				{
					results.append(qMakePair(mg->getObjectId(), prt->index()));
				}

			}
		}

		return results;
	}

	QList<ModelGroup*> getCurrentPrinterModelGroups()
	{
		return getPrinterMangager()->getPrinterModelGroups(currentPrinter()->index());
	}

	QList<ModelGroup*> getPrinterModelGroups(int index)
	{
		return getPrinterMangager()->getPrinterModelGroups(index);
	}

	QList<int> getPrinterModelGroupIds(int index)
	{
		QList<int> retGroupIds;

		for (ModelGroup* mg : getPrinterModelGroups(index))
		{
			if (mg)
			{
				retGroupIds << mg->getObjectId();
			}
		}

		return retGroupIds;
	}


	int getLockedPrintersNum()
	{
		auto pm = getPrinterMangager();
		int count = pm->printersCount();

		int num = 0;
		for (size_t i = 0; i < count; i++)
		{
			Printer* prt = pm->getPrinter(i);
			if (prt && prt->lock())
			{
				num++;
			}
		}

		return num;
	}

	int getFirstUnlockPrinterIndex()
	{
		//if (!needCheckPrinterLock())
		//	return 0;

		auto pm = getPrinterMangager();
		int count = pm->printersCount();

		for (size_t i = 0; i < count; i++)
		{
			Printer* prt = pm->getPrinter(i);
			if (prt && !prt->lock())
			{
				return i;
			}
		}

		qWarning() << "==== all printers are locked!!";
		return 0;
	}

	bool appendPrinters(int count)
	{
		return getPrinterMangager()->appendPrinters(count);
	}

	void removePrinters(int removeCount)
	{
		getPrinterMangager()->removePrinters(removeCount);
	}

	void updateAllPrinterContent()
	{
		getPrinterManager()->updatePrinterContent();
	}

	int getPrinterIndexByGroupId(int groupId)
	{
		return getPrinterManager()->getPrinterIndexByGroupId(groupId);
	}

	void checkGroupsToMove(int estimateBoxSize, QList<int64_t>& groupIds, QList<trimesh::dvec3>& offsets)
	{
		getPrinterMangager()->checkGroupsToMove(estimateBoxSize, groupIds, offsets);
	}

}