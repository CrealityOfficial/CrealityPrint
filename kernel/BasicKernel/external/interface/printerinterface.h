#ifndef PRINTERINTERFACE_1592732397175_H
#define PRINTERINTERFACE_1592732397175_H
#include "basickernelexport.h"
#include "data/header.h"
#include <QtCore/QList>

namespace qtuser_3d
{
	class Box3D;
}

namespace creative_kernel
{	
	class Printer;
	class ModelN;
	class ModelGroup;
	class PrinterMangager;

	BASIC_KERNEL_API int printersCount();
	BASIC_KERNEL_API Printer* getPrinter(int index);
	BASIC_KERNEL_API Printer* currentPrinter();
	BASIC_KERNEL_API qtuser_3d::Box3D currentPrinterGlobalBox();

	BASIC_KERNEL_API QList<QList<ModelGroup*>> getEachPrinterModelsGroups();
	BASIC_KERNEL_API QList<ModelGroup*> getAllPrinterModelsGroups();

	BASIC_KERNEL_API PrinterMangager* getPrinterManager();
	BASIC_KERNEL_API QList<ModelN*> getPrinterModels(int index);
	BASIC_KERNEL_API QList<ModelN*> getAllModelsOutOfPrinter();
	BASIC_KERNEL_API QList<int> getSlicedPrinterIndice();
	BASIC_KERNEL_API QByteArray getPrinterSerialData();
	BASIC_KERNEL_API void createPrinterFromProject(const QByteArray& data);

	BASIC_KERNEL_API bool appendPrinter(bool reversible = false);
	BASIC_KERNEL_API void removePrinter(int index, bool reversible = false);
	BASIC_KERNEL_API bool resizePrinters(int num);

	BASIC_KERNEL_API void setPrinterVisibility(int printerIndex, bool visibility, bool exclude = false);
	BASIC_KERNEL_API void showPrinter(int printerIndex = -1);
	BASIC_KERNEL_API void hidePrinter(int printerIndex = -1);

	BASIC_KERNEL_API void setPrinterPreviewMode(int printerIndex = -1);
	BASIC_KERNEL_API void setPrinterEditMode(int printerIndex = -1);

	BASIC_KERNEL_API void updateWipeTowers();

	BASIC_KERNEL_API bool hasModelsOnBorder(int index);

	BASIC_KERNEL_API Printer* getSelectedPrinter();
	BASIC_KERNEL_API void selectPrinter(Printer* p);
	BASIC_KERNEL_API void selectPrinter(int index, bool e = true);

	BASIC_KERNEL_API QString getAllWipeTowerPositionX(QString separate = ",");
	BASIC_KERNEL_API QString getAllWipeTowerPositionY(QString separate = ",");

	BASIC_KERNEL_API QList<qtuser_3d::Box3D> estimateGlobalBoxBeenLayout(const qtuser_3d::Box3D& box, int num);

	BASIC_KERNEL_API int currentPrinterIndex();
	BASIC_KERNEL_API int getModelPrinterIndex(ModelN* model);
	BASIC_KERNEL_API void getAllWipeTowerOutline(QList< std::vector<trimesh::vec3> >& outPathList, QList<int>& outIndexList);
	BASIC_KERNEL_API std::vector<trimesh::vec3> getPrinterWipeTowerOutline(int printerIndex);

	BASIC_KERNEL_API void updateAllPrinterContent();

	// ==== used when layout models
	BASIC_KERNEL_API QList< QPair<int, int> > getAllLockedPrinterModelGroupIds();
	BASIC_KERNEL_API QList<ModelGroup*> getCurrentPrinterModelGroups();
	BASIC_KERNEL_API QList<ModelGroup*> getPrinterModelGroups(int index);
	BASIC_KERNEL_API QList<int> getPrinterModelGroupIds(int index);
	BASIC_KERNEL_API int getLockedPrintersNum();
	BASIC_KERNEL_API int getFirstUnlockPrinterIndex();
	BASIC_KERNEL_API bool appendPrinters(int count);
	BASIC_KERNEL_API void removePrinters(int removeCount);
	BASIC_KERNEL_API void checkEmptyPrintersToRemove();
	BASIC_KERNEL_API int getPrinterIndexByGroupId(int groupId);
	BASIC_KERNEL_API void checkGroupsToMove(int estimateBoxSize, QList<int64_t>& groupIds, QList<trimesh::dvec3>& offsets);
	BASIC_KERNEL_API void checkEmptyPrintersToRemove();

	BASIC_KERNEL_API void resetPrinterNum();	//复位平台数目
	BASIC_KERNEL_API bool _checkModelInsideVisibleLayers(bool showAlert);
	// ==== used when layout models

}
#endif // PRINTERINTERFACE_1592732397175_H
