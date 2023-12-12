#ifndef _CREATIVE_BASE_UI_KERNEL_1589720896794_H
#define _CREATIVE_BASE_UI_KERNEL_1589720896794_H
#include "basickernelexport.h"
#include "cxkernel/kernel/cxkernel.h"

#include "data/kernelenum.h"
#include "kernel/globalconst.h"
#include <QtWidgets/QUndoStack>

class GLQuickItem;
class KernelUI;
class CaptureModelN;

namespace qtuser_3d
{
	class CameraController;
	class ScreenCamera;
	class RenderCenter;
	class QuickScene3DWrapper;
}

namespace qtuser_core
{
	class UndoProxy;
	class JobExecutor;
	class CreativePluginCenter;
	class ApplicationConfiguration;
	class CXFileOpenAndSaveManager;
}

namespace cxkernel
{
	class EventTracking;
}

namespace creative_kernel
{
	class VisualScene;
	class ModelSpaceUndo;
	class ModelSpace;
	class ReuseableCache;
	class PrinterEntityUpdater;
	class ModelNSelector;
	class PrinterNetMng;
	class ThemeNotifier;
	class PrinterNotifier;
	class CommandCenter;
	class DumpProxy;
	class Translator;
	class CusModelListModel;
	class KernelPhase;
	class SliceFlow;
	class RClickMenuList;
	class CusAddPrinterModel;
	class ParameterManager;
	class MaterialCenter;
	class SnapShot;
	class CADLoader;
	class BASIC_KERNEL_API Kernel : public cxkernel::CXKernel
	{
		Q_OBJECT
		Q_PROPERTY(int currentPhase READ currentPhase NOTIFY currentPhaseChanged)
	public:
		Kernel(QObject* parent = nullptr);
		virtual ~Kernel();

		QString entryQmlFile() override;
		void initializeContext() override;
		void initialize() override;
		void uninitialize() override;
		void loadUserSettings();

		int currentPhase();

		ReuseableCache* reuseableCache();
		qtuser_3d::RenderCenter* renderCenter();

		SliceFlow* sliceFlow();

		qtuser_3d::CameraController* cameraController();

		ModelSpace* modelSpace();
		CusModelListModel* listModel();
		ModelNSelector* modelSelector();

		VisualScene* visScene();

		ModelSpaceUndo* modelSpaceUndo();

		GlobalConst* globalConst();
		CommandCenter* commandCenter();
		cxkernel::EventTracking* sensorAnalytics();

		qtuser_core::ApplicationConfiguration* appConfiguration();
		ParameterManager* parameterManager();
		MaterialCenter* materialCenter();
		SnapShot* snapShot();

		KernelUI* kernelUI();
		void setSceneClearColor(const QColor &color);

		Q_INVOKABLE void openFile();
		Q_INVOKABLE bool checkUnsavedChanges();
		Q_INVOKABLE void processCommandLine();
		//Q_INVOKABLE void setGLQuickItem(GLQuickItem* quickItem);
		Q_INVOKABLE void setScene3DWrapper(qtuser_3d::QuickScene3DWrapper* item);

		Q_INVOKABLE void setKernelPhase(int phase);   //invoke from qml
		void setKernelPhase(KernelPhaseType phase);   //invoke from c++, maybe not ui thread

		Q_INVOKABLE void captureSceneByZproject();
		Q_SIGNAL void scenceVerticalViewUpdated();

	public slots:
		void viewChanged(bool requestUpdate);
	protected:
		void setKernelPhase(KernelPhase* phase);

	signals:
		void currentPhaseChanged();
	protected:
		ReuseableCache* m_reusableCache;
		ModelSpace* m_modelSpace;
		CusModelListModel* m_modelListModel;
		CusAddPrinterModel* m_addPrinterModel;

		qtuser_3d::RenderCenter* m_renderCenter;
		GlobalConst* m_globalConst;
		CommandCenter* m_commandCenter;
		SliceFlow* m_sliceFlow;
		CADLoader* m_cadLoader;

		qtuser_core::ApplicationConfiguration* m_configuration;

		qtuser_3d::CameraController* m_cameraController;

		VisualScene* m_visualScene;
		ModelNSelector* m_modelSelector;

		ModelSpaceUndo* m_modelSpaceUndo;

		KernelPhase* m_phases[3];
		KernelPhase* m_currentPhase;

		//internal
		ThemeNotifier* m_themeNotifier;
		PrinterNotifier* m_printerNotifier;

		DumpProxy* m_dumpProxy;

		cxkernel::EventTracking* m_sensorAnalytics;

		RClickMenuList* m_rclickMenuList;
		ParameterManager* m_parameterManager;
		MaterialCenter* m_materialCenter;

		SnapShot* m_snapShot;
		KernelUI* m_kernelUI;

		CaptureModelN *m_capture;
	};

	BASIC_KERNEL_API Kernel* getKernel();
	BASIC_KERNEL_API cxkernel::AppModule* createC3DContext();
}

#endif // _CREATIVE_BASE_UI_KERNEL_1589720896794_H
