#ifndef CREATIVE_KERNEL_PRINTER_202312131652_H
#define CREATIVE_KERNEL_PRINTER_202312131652_H

#include <QObject>
#include "basickernelexport.h"
#include "internal/render/platetexturecomponententity.h"
#include "qtuser3d/math/box3d.h"
#include "crslice2/crscene.h"
#include "external/data/interface.h"
#include <set>
#include <QTimer>

#include "crslice2/cacheslice.h"

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

		virtual void willSettingPrinter(Printer* p) = 0;

		virtual void willAutolayoutForPrinter(Printer* p) = 0;

		virtual void willPickBottomForPrinter(Printer* p) = 0;

		virtual void willEditNameForPrinter(Printer* p, QString from) = 0;

		virtual const WipeTowerGlobalSetting& wipeTowerCurrentSetting() = 0;

		virtual void localPrintSequenceChanged(Printer* p, QString newSequence) = 0;

		virtual QString globalPrintSequence() = 0;

		virtual QString globalPlateType() = 0;

		virtual void nameChanged(Printer* p, QString newName) = 0;
		virtual void printerIndexChanged(Printer* p, int newIndex) = 0;
		virtual void modelGroupsInsideChange(Printer* p, const QList<ModelGroup*>& list) = 0;
	};

	class BASIC_KERNEL_API Printer : public QObject, public SpaceTracer
	{
		Q_OBJECT
			Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
			Q_PROPERTY(bool dirty READ isDirty NOTIFY signalDirtyChanged)
			Q_PROPERTY(bool canSlice READ canSlice NOTIFY signalCanSliceChanged)
			Q_PROPERTY(int modelsInsideCount READ modelsInsideCount NOTIFY signalModelsInsideChange)
			Q_PROPERTY(QObject* settingsObject READ settingsObject CONSTANT)
			Q_PROPERTY(bool isSliced READ isSliced NOTIFY signalSlicedStateChanged)
			Q_PROPERTY(QObject* attain READ attain NOTIFY signalAttainChanged)
			Q_PROPERTY(QObject* sliceCache READ sliceCache NOTIFY signalAttainChanged)
			Q_PROPERTY(int index READ index WRITE setIndex NOTIFY signalIndexChanged)
			Q_PROPERTY(QString slicedPlateName READ slicedPlateName WRITE setSlicedPlateName NOTIFY sigSlicedPlateNameChanged)

			friend class OrcaCacheSlice;
	signals:
		void sigSlicedPlateNameChanged();
		void formModified();
		void nameChanged();
		void signalSelectPrinter(Printer* p);
		void signalIndexChanged();
		void signalModelsInsideChange(const QList<ModelN*>& list);

		void signalDirtyChanged();		// 发生任意变化
		void signalAttainChanged();		// 发生变化使得dirty从false变为true
		void signalSlicedStateChanged();

		void signalGCodeSettingsDirtyChanged();
		void signalCanSliceChanged();
		
	protected slots:
		void slotUpdateWipeTowerState();
		void slotWipeTowerPositionChange(QVector3D newPosition);
		void slotModelsDirtyChanged();
		void slotModelGroupsDirtyChanged();
		void setDirty(bool dirty);

	public slots:
		void requestPicture();
		void triggerCapture();

	public:

		Printer(PrinterEntity *entity, QObject* parent = nullptr);
		Printer(const Printer &other);
#ifndef Q_OS_WIN
		Printer() {}
#endif
		virtual ~Printer();

		void applyLayersColor(const QVariantList& layersColor);
		void reloadLayersConfig();
		void clearLayersConfig();
        crslice2::PlateInfo getPlateInfo();

		QString slicedPlateName();
		void setSlicedPlateName(const QString& name);

		bool isModifySlicePlateName();

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
		
		trimesh::dbox3 printerBox();
		const qtuser_3d::Box3D& box();
		qtuser_3d::Box3D globalBox();

		QVector3D position();
		void setPosition(const QVector3D& pos);

		const QString& name();
		void setName(const QString& str);

		void setModelNumber(const QString& str);

		void setTheme(int theme);

		void setWipeTowerPosition_LeftBottom(const QVector3D& pos);
		QVector3D getWipeTowerPosition_LeftBottom();

		QList<ModelN*> currentModelsInsideVisible();
		QList<ModelN*> modelsInsideVisible();

		const QList<ModelN*>& modelsInside();
		void replaceModelsInside(QList<ModelN*>& list);

		const QList<ModelGroup*>& modelGroupsInside();
		void replaceModelGroupsInside(QList<ModelGroup*>& list);

		int modelsInsideCount();

		const QList<ModelN*>& modelsOnBorder();
		void replaceModelsOnBorder(QList<ModelN*>& list);
		bool hasModelsOnBorder();

		const QList<ModelGroup*>& modelGroupsOnBorder();
		void replaceModelGroupsOnBorder(QList<ModelGroup*>& list);
		bool hasModelGroupsOnBorder();

		QString primeTowerPosition(int axis);

		void setVisibility(bool visibility);
		bool isVisible();

		void setCloseEnable(bool enable);

		trimesh::dbox3 modelsInsideBoundingBox();

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
		bool checkBedTypeInvalid();
		/*
			includeSpecificExtruder:是否包含编辑gcode耗材丝产生的颜色索引
		*/
		QList<QVector4D> getModelInsideUseColors(bool includeSpecificExtruder = false);
		QSet<int> getUsedExtruders(bool includeSpecificExtruder = false);

		bool isDirty();
		void resetDirty();
		void dirty();

		bool isGCodeSettingDirty();
		void resetGCodeSettingDirty();
		void gCodeSettingDirty();

		bool canSlice();

		//for slice and preview
		QString pictureUrl();
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

		void onModelGroupModified(ModelGroup* _model_group, const QList<ModelN*>& removes, const QList<ModelN*>& adds);

		void updateAssociateModelsState();
		QString generateSceneName();

		void ResetSettings();

	protected:
		void handleClickEvents(IconTypes type);
		
		qtuser_3d::Box3D getWipeTowerBox(QList<QVector4D>& colors);

		void onLocalPlateTypeChanged(QString str);
		void setPlateStyle(QString str);

		Printer(QObject* parent = nullptr);
	private:
		QString m_SlicedPlateName = QString();
		int m_index;
		qtuser_3d::Box3D m_box;

		QVector3D m_pos;

		PrinterEntity* m_entity;
		PlateEntity* m_plate;

		PrinterTracer* m_tracer;

		// QList<ModelN*> m_modelsInsideVisible;
		QList<ModelN*> m_modelsInside;
		QList<ModelN*> m_modelsOnBorder;

		QList<ModelGroup*> m_groupsInside;
		QList<ModelGroup*> m_groupsOnBorder;

		QList<QString> m_insideModelSerialNames;

		PrinterSettings* m_settings;

		bool m_isDirty { true };
		bool m_isGCodeSettingDirty { true };

		SliceInfo m_sliceInfo;
		bool m_reserveSliceCacheFlag { false };

		QTimer m_requestPictureTimer;
		bool m_pictureFlag{ false };
		int m_pictureIndex = 0;

		crslice2::CrSlicePrint m_crslice_print;
	public:
		static bool checkTwoListEqual(const QList<ModelN*>& a, const QList<ModelN*>& b);
		static bool checkTwoGroupListEqual(const QList<ModelGroup*>& a, const QList<ModelGroup*>& b);
	};
}


#endif // !CREATIVE_KERNEL_PRINTER_202312131652_H
