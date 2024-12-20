#ifndef CREATIVE_KERNEL_PRINTER_MANAGER_202312131655_H
#define CREATIVE_KERNEL_PRINTER_MANAGER_202312131655_H

#include "printer.h"
#include "qtuser3d/math/box3d.h"
#include "external/data/interface.h"
#include "qtuser3d/event/eventhandlers.h"

namespace creative_kernel
{
	class ModelN;
	class ModelGroup;
	class WipeTowerEntity;
	class PMCollisionCheckHandler;

	class BASIC_KERNEL_API PrinterMangager : public QObject , public PrinterTracer, public SpaceTracer
	{
		Q_OBJECT
		Q_PROPERTY(QObject* selectedPrinter READ getSelectedPrinterObject NOTIFY signalDidSelectPrinter)
		Q_PROPERTY(QList<int> slicedPrinters READ slicedPrinters NOTIFY slicedPrintersChanged)
		Q_PROPERTY(int selectedPrinterIndex READ selectedPrinterIndex WRITE setSelectedPrinterIndex NOTIFY signalDidSelectPrinter)
		Q_PROPERTY(int printersCount READ printersCount NOTIFY signalPrintersCountChanged)
		Q_PROPERTY(bool hasPrinterSliced READ hasPrinterSliced NOTIFY sigHasPrinterSlicedChanged)

	public:
		PrinterMangager(QObject* parent = nullptr);
		virtual ~PrinterMangager();

		void initialize();
		void setTracer(PrinterMangagerTracer *tracer);

		Printer* calculateModelGroupPrinter(ModelGroup* _model_group);
		QList<ModelGroup*> getPrinterModelGroups(Printer* printer);
		QList<QList<ModelGroup*>> getEachPrinterModelsGroups();
		QList<ModelGroup*> getAllPrinterModelsGroups();
		
		QList<int> slicedPrinters();

		bool addPrinter(Printer* p);
		bool insertPrinter(int index, Printer* p);

		bool resizePrinters(int num);

		void deletePrinter(int idx, bool isCompletely = true);
		void deletePrinter(Printer* p, bool isCompletely = true);

		bool hasPrinterSliced();

		bool printerEnough();
		int printersCount();
		Printer* getPrinter(int index);

		void selectPrinter(Printer* p);
		Q_INVOKABLE bool selectPrinter(int index, bool exclude = true);
		Printer* getSelectedPrinter();
		QObject* getSelectedPrinterObject();

		int selectedPrinterIndex();
		void setSelectedPrinterIndex(int index);

		int indexOfPrinter(Printer* printer);

		void allPrinterUpdateBox(const qtuser_3d::Box3D& box);

		Q_INVOKABLE QList<int> usedExtruders();
		Q_INVOKABLE bool hasSliced();

		QList<ModelN*> getPrinterModels(int index);
		bool hasModelsOnBorder(int index);

		//包括不在任何一个盘内及在盘边上的模型
		QList<ModelN*> getAllModelsOutOfPrinter();

		//包括不在任何一个盘内及在盘边上的模型组
		QList<ModelGroup*> getAllModelGroupsOutOfPrinter();

		QList<ModelGroup*> getPrinterModelGroups(int index);
		QList<int> getSlicedPrinterIndice();

		const qtuser_3d::Box3D& boundingBox();
		qtuser_3d::Box3D extendBox();
		void refittingExtendBox();
		void relayout();

		void setTheme(int theme);

		void setPrinterVisibility(int printerIndex, bool visibility, bool exclude = false);
		void showPrinter(int printerIndex = -1);
		void hidePrinter(int printerIndex = -1);

		static QList<qtuser_3d::Box3D> estimateGlobalBoxBeenLayout(const qtuser_3d::Box3D& box, int num);

		// Wipe Tower hide, plate style change
		void setPrinterPreviewMode(int printerIndex = -1);
		void setPrinterEditMode(int printerIndex = -1);

		QList<WipeTowerEntity*>wipeTowerEntities();
		Printer* findPrinterFromWipeTower(WipeTowerEntity*t);

		void updateWipeTowers();
		void updateWipeTowersPositions(const std::vector<float> x, const std::vector<float> y);

		QString getAllWipeTowerPosition(int axis, QString separate = ",");

		void checkModelsInSelectedPrinterCollision();

		void checkSelectedPrinterModelsOnBorder();

		// used when layout models
		bool appendPrinters(int count);
		void removePrinters(int removeCount, bool isCompletely = true);
		void checkEmptyPrintersToRemove();
		// used when layout models

		//save/load project
		QByteArray getSerialData();

		// ==== used in undoStack
		bool insertPrinterEx(int index, Printer* p, QList<int64_t>& moveGroupIds, QList<trimesh::dvec3>& offsets);
		void deletePrinterEx(Printer* p, QList<int64_t>& moveGroupIds, QList<trimesh::dvec3>& offsets, bool isCompletely = true);
		void checkGroupsToMove(int estimateBoxSize, QList<int64_t>& groupIds, QList<trimesh::dvec3>& offsets);
		// ==== used in undoStack

		int getPrinterIndexByGroupId(int groupId);

		void updatePrinterContent();
	public slots:
		void dirtyAllPrinters();
		void onGlobalSettingsChanged(QObject* parameter_base, const QString& key);

	signals:
		void signalPrintersCountChanged();
		void signalAddPrinter(Printer* p);
		void signalDeletePrinter(Printer* p);
		void signalModelsOutOfPrinterChange(const QList<ModelN*>& list);
		void signalDidSelectPrinter(Printer* p);
		void slicedPrintersChanged();
		void sigHasPrinterSlicedChanged();

	protected:
		void layout();

		void notifyAddedPrinter(Printer* p);
		void notifyDeletePrinter(Printer* p);
		void notifySelectPrinter(Printer* p);

		void notifyPrinter(Printer* p);
		void disNodifyPrinter(Printer* p);

		void onPrintersCountChanged();

		void groupModelsForPrinter(Printer* pt, QList<ModelN *>&models, QList<int>&groupedFlags);
		void groupModelsByPrinterGlobalBox();

		void divideModelGroupsForPrinter(Printer* pt, QList<ModelGroup*>& groups, QList<int>& groupedFlags);
		void divideModelGroupsByPrinterGlobalBox();

		// for Wipe Tower
		void onWipeTowerEnable(bool enable); //global state
		void onLayerHeightChanged(float height);
		void onWipeTowerVolumeChanged(float volume);
		void onWipeTowerWidthChanged(float width);
		void onSupportEnable(bool enable);
		void onSupportFilamentChanged(int index);
		void onSupportInterfaceFilamentChanged(int index);

		// for print sequence
		void onGlobalPrintSequenceChanged(const QString& str);
		void handlePrintSequence(const QString& str);

		void onGlobalPlateTypeChanged(const QString& str);

	protected:
		virtual bool shouldDeletePrinter(Printer* p) override;
		virtual void willDeletePrinter(Printer* p) override;
		virtual void willSelectPrinter(Printer* p) override;
		virtual void didSelectPrinter(Printer* p) override;

		virtual void didLockPrinterOrNot(Printer* p, bool lockOrNot) override;

		//virtual void didBoundingBoxChange(Printer* p, const qtuser_3d::Box3D& newBox) override;

		virtual void willSettingPrinter(Printer* p) override;

		virtual void willAutolayoutForPrinter(Printer* p) override;

		virtual void willPickBottomForPrinter(Printer* p) override;

		virtual void willEditNameForPrinter(Printer* p, QString from) override;

		virtual const WipeTowerGlobalSetting& wipeTowerCurrentSetting() override;

		//for print sequence:print by object or layer
		virtual void localPrintSequenceChanged(Printer* p, QString newSequence) override;

		virtual QString globalPrintSequence() override;

		virtual QString globalPlateType() override;

		virtual void nameChanged(Printer* p, QString newName) override;
		virtual void printerIndexChanged(Printer* p, int newIndex) override;
		virtual void modelGroupsInsideChange(Printer* p, const QList<ModelGroup*>& list) override;


	protected:
		virtual void onSceneChanged(const trimesh::dbox3& box) override;

		Q_INVOKABLE bool appendPrinter();

		void viewAllPrintersFromTop();

	private:
		QList<Printer *> m_printers;
		qtuser_3d::Box3D m_boundingBox;

		PrinterMangagerTracer* m_tracer;

		//包括不在任何一个盘内及在盘边上的模型
		QList<ModelN*> m_modelsOutOfPrinter;
		QList<ModelGroup*> m_modelGroupsOutOfPrinter;

		WipeTowerGlobalSetting m_wipeTowerSetting;

		PMCollisionCheckHandler* m_collisionCheckHandler;
	};

	PrinterMangager* getPrinterMangager();
}

#endif // CREATIVE_KERNEL_PRINTER_MANAGER_202312131655_H
