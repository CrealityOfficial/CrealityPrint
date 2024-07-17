#ifndef CREATIVE_KERNEL_PRINTER_202312131652_H
#define CREATIVE_KERNEL_PRINTER_202312131652_H

#include <QObject>
#include "basickernelexport.h"
#include "internal/render/platetexturecomponententity.h"
#include "qtuser3d/math/box3d.h"
#include "crslice2/crscene.h"

namespace creative_kernel
{
	class PrinterSettings;
	class PrinterEntity;
	class PlateEntity;
    class SliceAttain;
	class Printer;
	class ModelN;
	class ModelGroup;

	struct SliceInfo
	{
		QImage picture;
		bool isSliced{ false };
        QObject* attain { NULL };
        QObject* sliceCache { NULL };
		QString sliceError;
	};

	class WipeTowerGlobalSetting
	{
	public:
		bool enable;
		float layerHeight;
		float volume;
		float width;
		bool enableSupport;
		int supportFilamentColorIndex;
		int supportInterfaceFilamentColorIndex;
	};

	class PrinterTracer
	{
	public:
		virtual ~PrinterTracer() {}

		virtual bool shouldDeletePrinter(Printer* p) = 0;
		virtual void willDeletePrinter(Printer* p) = 0;

		virtual void willSelectPrinter(Printer* p) = 0;
		virtual void didSelectPrinter(Printer* p) = 0;

		virtual void didLockPrinterOrNot(Printer* p, bool lockOrNot) = 0;

		//virtual void didBoundingBoxChange(Printer* p, const qtuser_3d::Box3D& newBox) = 0;

		virtual void willSettingPrinter(Printer* p) = 0;

		virtual void willAutolayoutForPrinter(Printer* p) = 0;

		virtual void willPickBottomForPrinter(Printer* p) = 0;

		virtual void willEditNameForPrinter(Printer* p, QString from) = 0;

		virtual void willModifyPrinter(Printer* p) = 0;

		virtual const WipeTowerGlobalSetting& wipeTowerCurrentSetting() = 0;

		virtual void localPrintSequenceChanged(Printer* p, QString newSequence) = 0;

		virtual QString globalPrintSequence() = 0;

		virtual QString globalPlateType() = 0;
	};

	class BASIC_KERNEL_API Printer : public QObject
	{
	Q_OBJECT
        Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
		Q_PROPERTY(bool dirty READ isDirty NOTIFY signalDirtyChanged)
		Q_PROPERTY(int modelsInsideCount READ modelsInsideCount NOTIFY signalModelsInsideChange)
		Q_PROPERTY(QObject* settingsObject READ settingsObject CONSTANT)
		Q_PROPERTY(bool isSliced READ isSliced NOTIFY signalSlicedStateChanged)
		Q_PROPERTY(QObject* attain READ attain NOTIFY signalAttainChanged)
		Q_PROPERTY(QObject* sliceCache READ sliceCache NOTIFY signalAttainChanged)
		Q_PROPERTY(int index READ index WRITE setIndex NOTIFY signalIndexChanged)

	signals:
		void nameChanged();
		void signalModelsInsideChange(const QList<ModelN*>& list);
		void signalSelectPrinter(Printer* p);
		void signalDirtyChanged();		// 发生任意变化
		void signalAttainChanged();		// 发生变化使得dirty从false变为true
		void signalSlicedStateChanged();
		void signalIndexChanged();

	protected slots:
		void slotWipeTowerPositionChange(QVector3D newPosition);
		void slotModelsDirtyChanged();
		void setDirty(bool dirty);

	public:

		Printer(PrinterEntity *entity, QObject* parent = nullptr);
		Printer(const Printer &other);
		virtual ~Printer();

		void applyLayersColor(const QVariantList& layersColor);
		void reloadLayersConfig();
		void clearLayersConfig();
        crslice2::plateInfo getPlateInfo();

		void setPrinterEditMode();
		void setPrinterPreviewMode(); //for gcode preview

		PrinterEntity* entity();

		void addTracer(PrinterTracer* tracer);

		bool selected();
		void setSelected(bool selected);

		int index();
		void setIndex(int idx);

		bool lock();
		void setLock(bool lock);

		void updateBox(const qtuser_3d::Box3D& box);
		const qtuser_3d::Box3D& box();
		qtuser_3d::Box3D globalBox();

		QVector3D position();
		void setPosition(const QVector3D& pos);

		const QString& name();
		void setName(const QString& str);

		void setModelNumber(const QString& str);

		void setTheme(int theme);

		QList<ModelN*> currentModelsInsideVisible();
		QList<ModelN*> modelsInsideVisible();

		const QList<ModelN*>& modelsInside();
		void replaceModelsInside(QList<ModelN*>& list);

		int modelsInsideCount();

		const QList<ModelN*>& modelsOnBorder();
		void replaceModelsOnBorder(QList<ModelN*>& list);
		bool hasModelsOnBorder();

		void setVisibility(bool visibility);
		bool isVisible();

		void setCloseEnable(bool enable);

		qtuser_3d::Box3D modelsInsideBoundingBox();

		bool checkAndUpdateWipeTowerState();

		bool checkWipeTowerCollide(bool showAlert = true);
		bool checkWipeTowerShouldShow();
		void adjustWipeTowerPositionWhenInvalid();
		void moveWipeTower(const QVector3D& delta);
		void updateWipeTowerState();
		std::vector<trimesh::vec3> wipeTowerOutlinePath(bool global);

		QString getLocalPrintSequence();
		//综合考虑的当前盘全局及本地打印序列
		QString getPrintSequence();

		QString getLocalPlateType();
		void onGlobalPlateTypeChanged(QString str);

		bool checkModelInsideVisibleLayers(bool showAlert);
		bool checkVisiableModelError();

		QList<QVector4D> getModelInsideUseColors();

		bool isDirty();
		void resetDirty();
		void dirty();

		//for slice and preview
		QImage picture() const;
		void setPicture(const QImage& p);

		bool isSliced();
		void setIsSliced(bool isSliced);

		void reserveSliceCache();
		void clearSliceCache();

		QObject* sliceCache();
		QObject* attain();
		void setAttain(SliceAttain* attain);

		void applySettings();
		Q_SIGNAL void settingsApplyed() const;
		Q_INVOKABLE void setParameter(const QString& key, const QString& value);
		Q_INVOKABLE QString parameter(const QString& key);
		Q_SIGNAL void parameterChanged(const QString& key, const QString& value) const;
		Q_INVOKABLE bool hasModelInsideUseColors();
		QObject* settingsObject();
		std::map<std::string, std::string> stdSettings();

		void setSliceError(const QString& error);
		void clearSliceError();
		void checkSliceError();

		void disconnectModels();

	protected:
		void handleClickEvents(IconTypes type);

		void updateAssociateModelsState();
		
		qtuser_3d::Box3D getWipeTowerBox(QList<QVector4D>& colors);

		void onLocalPlateTypeChanged(QString str);
		void setPlateStyle(QString str);

		Printer(QObject* parent = nullptr);
	private:

		int m_index;
		qtuser_3d::Box3D m_box;

		QVector3D m_pos;

		PrinterEntity* m_entity;
		PlateEntity* m_plate;

		PrinterTracer* m_tracer;

		// QList<ModelN*> m_modelsInsideVisible;
		QList<ModelN*> m_modelsInside;
		QList<ModelN*> m_modelsOnBorder;

		QList<QString> m_insideModelSerialNames;

		PrinterSettings* m_settings;

		bool m_isDirty { true };

		SliceInfo m_sliceInfo;
		bool m_reserveSliceCacheFlag { false };

	public:
		static bool checkTwoListEqual(const QList<ModelN*>& a, const QList<ModelN*>& b);
	};
}


#endif // !CREATIVE_KERNEL_PRINTER_202312131652_H
