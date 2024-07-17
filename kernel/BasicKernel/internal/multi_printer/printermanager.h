#ifndef CREATIVE_KERNEL_PRINTER_MANAGER_202312131655_H
#define CREATIVE_KERNEL_PRINTER_MANAGER_202312131655_H

#include "printer.h"
#include "qtuser3d/math/box3d.h"
#include "external/data/interface.h"
#include "qtuser3d/event/eventhandlers.h"

namespace creative_kernel
{
	class ModelN;
	class WipeTowerEntity;
	class PMCollisionCheckHandler;

	class PrinterMangagerTracer
	{
	public:
		virtual ~PrinterMangagerTracer() {}

		virtual void willSettingPrinter(Printer* p) = 0;
		
		virtual void willAutolayoutForPrinter(Printer* p) = 0;

		virtual void willPickBottomForPrinter(Printer* p) = 0;

		virtual void willEditNameForPrinter(Printer* p, QString from) = 0;

		virtual void willModifyPrinter(Printer* p) = 0;

		virtual void message(Printer* p, int type, QString msg) = 0;
	};


	class BASIC_KERNEL_API PrinterMangager : public QObject , public PrinterTracer, public SpaceTracer
	{
		Q_OBJECT
		Q_PROPERTY(QObject* selectedPrinter READ getSelectedPrinterObject NOTIFY signalDidSelectPrinter)
		Q_PROPERTY(int printersCount READ printersCount NOTIFY signalPrintersCountChanged)

	public:
		PrinterMangager(QObject* parent = nullptr);
		virtual ~PrinterMangager();

		void initialize();

		void setTracer(PrinterMangagerTracer *tracer);

		bool addPrinter(Printer* p);
		bool insertPrinter(int index, Printer* p);
		
		bool resizePrinters(int num);

		void deletePrinter(int idx, bool isCompletely = true);
		void deletePrinter(Printer* p, bool isCompletely = true);

		bool printerEnough();
		int printersCount();
		Printer* getPrinter(int index);

		bool selectPrinter(int index, bool exclude = true);
		Printer* getSelectedPrinter();
		QObject* getSelectedPrinterObject();

		void allPrinterUpdateBox(const qtuser_3d::Box3D& box);

		QList<ModelN*> getPrinterModels(int index);
		bool hasModelsOnBorder(int index);
		QList<ModelN*> getAllModelsOutOfPrinter();

		const qtuser_3d::Box3D& boundingBox();
		qtuser_3d::Box3D extendBox();
		void refittingExtendBox();

		void setTheme(int theme);

		void setPrinterVisibility(int printerIndex, bool visibility, bool exclude = false);
		void showPrinter(int printerIndex = -1);
		void hidePrinter(int printerIndex = -1);
		void groupModelsByPrinterGlobalBox();
		static QList<qtuser_3d::Box3D> estimateGlobalBoxBeenLayout(const qtuser_3d::Box3D& box, int num);

		// Wipe Tower hide, plate style change 
		void setPrinterPreviewMode(int printerIndex = -1);
		void setPrinterEditMode(int printerIndex = -1);

		QList<WipeTowerEntity*>wipeTowerEntities();
		void updateWipeTowers();
		void updateWipeTowersPositions(float x, float y);

		QString getAllWipeTowerPosition(int axis, QString separate = ",");

		void checkModelsInSelectedPrinterCollision();

		void checkSelectedPrinterModelsOnBorder();

		// used when layout models
		bool appendPrinters(int count);
		void removePrinters(int removeCount, bool isCompletely = true);
		// used when layout models

		//save/load project
		QByteArray getSerialData();

		// ==== used in undoStack
		bool insertPrinterEx(int index, Printer* p, QList<QString>& moveModelSerialNames, QList<QVector3D>& startPoses, QList<QVector3D>& endPoses);
		void deletePrinterEx(Printer* p, QList<QString>& moveModelSerialNames, QList<QVector3D>& starts, QList<QVector3D>& ends, bool isCompletely = true);
		// ==== used in undoStack

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

	protected:
		void layout();

		void onPrintersCountChanged();

		void groupModelsForPrinter(Printer* pt, QList<ModelN *>&models, QList<int>&groupedFlags);


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

		void checkModelsToMove(int estimateBoxSize, QList<ModelN*>& modelsToMove, QList<QVector3D>& startPoses, QList<QVector3D>& endPoses);

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

		virtual void willModifyPrinter(Printer* p) override;

		virtual const WipeTowerGlobalSetting& wipeTowerCurrentSetting() override;

		//for print sequence:print by object or layer
		virtual void localPrintSequenceChanged(Printer* p, QString newSequence) override;

		virtual QString globalPrintSequence() override;

		virtual QString globalPlateType() override;

	protected:
		virtual void onSceneChanged(const qtuser_3d::Box3D& box) override;
		virtual void onBoxChanged(const qtuser_3d::Box3D& box) override;		

		Q_INVOKABLE bool appendPrinter();

		void viewAllPrintersFromTop();

	private:
		QList<Printer *> m_printers;
		qtuser_3d::Box3D m_boundingBox;

		PrinterMangagerTracer* m_tracer;

		//包括不在任何一个盘内及在盘边上的模型
		QList<ModelN*> m_modelsOutOfPrinter;

		WipeTowerGlobalSetting m_wipeTowerSetting;

		PMCollisionCheckHandler* m_collisionCheckHandler;
	};

	PrinterMangager* getPrinterMangager();
}

#endif // CREATIVE_KERNEL_PRINTER_MANAGER_202312131655_H


