#include "kernel.h"

#include "data/modelspace.h"
#include "kernel/modelnselector.h"
#include "qtusercore/module/systemutil.h"

#include "qtuser3d/camera/cameracontroller.h"
#include "qtusercore/util/undoproxy.h"
#include "qtusercore/module/jobexecutor.h"
#include "qtusercore/util/applicationconfiguration.h"
#include "qtusercore/module/cxopenandsavefilemanager.h"
#include <qtusercore/string/resourcesfinder.h>

#if USE_CXCLOUD
#include <cxcloud/net/http_request.h>
#include <cxcloud/service_center.h>
#endif
#include "kernel/visualscene.h"
#include "kernel/rendercenter.h"

#include "data/modelspaceundo.h"
#include "interface/renderinterface.h"
#include "interface/eventinterface.h"
#include "interface/spaceinterface.h"
#include "interface/uiinterface.h"
#include "interface/commandinterface.h"
#include "interface/reuseableinterface.h"
#include "interface/cloudinterface.h"
#include "interface/appsettinginterface.h"
#include "interface/selectorinterface.h"
#include "cxkernel/interface/iointerface.h"
#include "interface/machineinterface.h"
#include "cxkernel/interface/modelninterface.h"

#include "kernel/reuseablecache.h"
#include "kernel/commandcenter.h"
#include "kernel/translator.h"
#include "kernel/sensoranalytics.h"
#include "kernel/snapshot.h"
#include "kernel/kernelui.h"
#include "kernel/capturemodeln.h"

#include "internal/utils/dumpproxy.h"
#include "internal/render/printerentity.h"
#include "internal/utils/printernotifier.h"
#include "internal/utils/themenotifier.h"
#include "internal/data/cusModelListModel.h"
#include "internal/data/cusaddprintermodel.h"
#include "internal/rclick_menu/rclickmenulist.h"
#include "internal/parameter/parametermanager.h"
#include "internal/parameter/materialcenter.h"
#include "internal/utils/cadloader.h"

#include "localnetworkinterface/devicephase.h"

#include "utils/namedeclare.h"

#include "slice/sliceflow.h"

#include "qtusercore/module/creativeplugincenter.h"
#include "qtuserqml/updater/UpdateManager.h"
#include "qtuserqml/gl/glquickitem.h"
#include "qtuserqml/macro.h"

#include <QtQml/QQmlEngine>
#include <QtQml/QQmlContext>
#include <QtCore/QCoreApplication>
#include <QtCore/QFileInfo>

#include "cxffmpeg/qmlplayer.h"
#include "qcxchart.h"

#define CXSW_SCOPE "com.cxsw.SceneEditor3D"
#define VERSION_MAJOR 1
#define VERSION_MINOR 0
#define COMMON_REASON "Created by Cpp"

#define CXSW_REG(x) QML_INTERFACE(x, CXSW_SCOPE, VERSION_MAJOR, VERSION_MINOR)
#define CXSW_REG_U(x) QML_INTERFACE_U(x, CXSW_SCOPE, VERSION_MAJOR, VERSION_MINOR, COMMON_REASON)

namespace creative_kernel
{
	Kernel* gKernel = nullptr;
	Kernel* getKernel()
	{
		return gKernel;
	}

	qtuser_quick::AppModule* createC3DContext()
	{
		return new Kernel();
	}

	Kernel::Kernel(QObject* parent)
		: CXKernel{
				[this]() {
					m_globalConst = new GlobalConst{ this };
					return m_globalConst;
				},
				parent,
			}
		, m_cameraController(nullptr)
		, m_visualScene(nullptr)
		, m_currentPhase(nullptr)
		, m_capture(nullptr)
	{
		gKernel = this;

		m_commandCenter = new CommandCenter(this);

		m_configuration = new qtuser_core::ApplicationConfiguration(this);
		m_configuration->registerBundle(global_render_bundle);
		m_configuration->registerBundle(plugin_render_bundle);
		m_configuration->registerBundle(global_render_light_bundle);
		m_configuration->registerBundle(plugin_render_light_bundle);
		m_configuration->registerBundle(default_config_bundle);

		m_modelSpace = new ModelSpace(this);
		m_modelListModel = new CusModelListModel(this);
		m_addPrinterModel = new CusAddPrinterModel();

		m_renderCenter = new RenderCenter(this);

		m_cameraController = new qtuser_3d::CameraController(this);
		m_cameraController->setEnableZoomAroundCursor(true);

		m_kernelUI = new KernelUI(this);
		m_visualScene = new VisualScene();

		m_modelSelector = new ModelNSelector(this);
		m_modelSelector->setPickerSource(m_visualScene->facePicker());

		m_modelSpaceUndo = new ModelSpaceUndo(this);

		m_reusableCache = new ReuseableCache(m_visualScene);

		m_printerNotifier = new PrinterNotifier(this);
		m_themeNotifier = new ThemeNotifier(this);

		m_dumpProxy = new DumpProxy(this);

		m_sliceFlow = new SliceFlow(this);
		m_sensorAnalytics = new SensorAnalytics(this);

		m_phases[0] = m_visualScene;
		m_phases[1] = m_sliceFlow;
		m_phases[2] = new DevicePhase(this);

		m_rclickMenuList = new RClickMenuList(this);
		m_parameterManager = new ParameterManager(this);
		m_materialCenter = new MaterialCenter(this);

		m_snapShot = new SnapShot(this);
		m_cadLoader = new CADLoader(this);

		m_capture = new CaptureModelN(this);
		m_capture->setRequestCallback([this](auto) { Q_EMIT scenceVerticalViewUpdated(); });
	}

	Kernel::~Kernel()
	{
	}

	QString Kernel::entryQmlFile()
	{
		return QLatin1String("qrc:/scence3d/res/main.qml");
	}

	void Kernel::initialize()
	{
		qDebug() << "kernel initialize";

#if USE_CXCLOUD
		cxcloud()->initialize();
		cxcloud()->setApplicationType(cxcloud::ApplicationType::CREATIVE_PRINT);
		cxcloud()->setOpenFileHandler(cxkernel::openFileWithName);
		cxcloud()->setOpenFileListHandler(cxkernel::openFileWithNames);
		cxcloud()->setSelectedModelCombineSaver([this](const QString& dir_path) {
			QString file_name;
			for (const auto* model : selectionms()) {
				auto model_name = model->objectName().replace(QStringLiteral(".stl"), QStringLiteral(""));
				file_name.append(QStringLiteral("%1-").arg(std::move(model_name)));
			}
			file_name = file_name.left(file_name.length() - 1);
			auto file_path = QStringLiteral("%1/%2.stl").arg(dir_path).arg(std::move(file_name));
			cmdSaveAs(file_path);
			return file_path;
		});
		cxcloud()->setSelectedModelUncombineSaver([this](const QString& dir_path) {
			QList<QString> file_path_list;
			cmdSaveSelectStl(dir_path, file_path_list);
			for (auto& file_path : file_path_list) {
				file_path = QStringLiteral("%1/%2").arg(dir_path).arg(file_path);
			}
			return file_path_list;
		});
		cxcloud()->setCurrentSliceSaver([this](const QString& dir_path) {
			return m_sliceFlow->saveCurrentGcodeToDir(dir_path);
		});
		cxcloud()->setVaildSliceFileSuffixList({ QStringLiteral("gcode") });
		cxcloud()->setOnlineSliceFileCompressed(true);
		cxcloud()->setModelCacheDirPath(QFileInfo{
			qtuser_core::getOrCreateAppDataLocation(QStringLiteral("../../%1_%2")
				.arg(QStringLiteral(PROJECT_NAME)).arg(QStringLiteral("cloud_models")))
		}.absoluteFilePath());
		cxcloud()->setMaxDownloadThreadCount(8);
#endif

		m_sliceFlow->initialize();
		m_dumpProxy->initialize();

		m_cameraController->setScreenCamera(m_reusableCache->mainScreenCamera());
		m_reusableCache->intialize();
		cxkernel::registerOpenHandler(m_cadLoader);

		connect(m_cameraController, SIGNAL(signalViewChanged(bool)), this, SLOT(viewChanged(bool)));

		m_modelSpace->initialize();
		m_modelSpace->addSpaceTracer(m_printerNotifier);
		m_modelSelector->addTracer(m_modelListModel);
		m_modelSelector->addTracer(m_printerNotifier);

		m_reusableCache->mainScreenCamera()->addCameraObserver(m_printerNotifier);

		addResizeEventHandler(m_cameraController);
		addRightMouseEventHandler(m_cameraController);
		addMidMouseEventHandler(m_cameraController);
		addWheelEventHandler(m_cameraController);
		addHoverEventHandler(m_cameraController);

		qtuser_3d::Box3D baseBox = getModelSpace()->baseBoundingBox();
		m_cameraController->home(baseBox, 1);

		m_visualScene->initialize();

		m_addPrinterModel->initialize();
		registerRenderGraph(m_visualScene);
		addUIVisualTracer(m_themeNotifier);

		m_materialCenter->intialize();
		m_parameterManager->intialize();

		qDebug() << "kernel initialize over";
		showSysMemory();

		m_kernelUI->initialize();

		loadUserSettings();
		m_kernelUI->loadUserSettings();
	}

	void Kernel::uninitialize()
	{
		m_kernelUI->uninitialize();

		m_engine->removeImageProvider(QStringLiteral("scence_vertical_view_provider"));

		qDebug() << "kernel uninitialize start";
		m_renderCenter->uninitialize();
		m_modelSpace->uninitialize();
#if USE_CXCLOUD
		cxcloud()->uninitialize();
#endif

		renderRenderGraph(nullptr);
		m_reusableCache->blockRelation();
		m_sliceFlow->uninitialize();

		qDebug() << "kernel uninitialize over";
	}

	void Kernel::initializeContext()
	{
		CXSW_REG(GLQuickItem);
		CXSW_REG(QMLPlayer);
		CXSW_REG(QCxChart);

		CXSW_REG_U(ToolCommand);
		CXSW_REG_U(ActionCommand);

		m_engine->addImageProvider(QStringLiteral("scence_vertical_view_provider"),
															 m_capture->createImageProvider());

		m_kernelUI->registerQmlEngine(*m_engine);
		cxkernel::setModelNDataProcessor(m_modelSpace);

		m_context->setContextProperty("kernel_kernel", this);
		m_context->setContextProperty("kernel_global_const", m_globalConst);
		m_context->setContextProperty("kernel_command_center", m_commandCenter);
		m_context->setContextProperty("kernel_render_center", m_renderCenter);
		m_context->setContextProperty("kernel_reusable_cache", m_reusableCache);
		m_context->setContextProperty("kernel_modelspace", m_modelSpace);
		m_context->setContextProperty("kernel_model_selector", m_modelSelector);
		m_context->setContextProperty("kernel_model_list_data", m_modelListModel);
		m_context->setContextProperty("kernel_camera_controller", m_cameraController);
		m_context->setContextProperty("kernel_undo_proxy", m_undoProxy);
		m_context->setContextProperty("kernel_io_manager", m_ioManager);
		m_context->setContextProperty("kernel_slice_flow", m_sliceFlow);
		m_context->setContextProperty("kernel_sensor_analytics", m_sensorAnalytics);
		m_context->setContextProperty("kernel_rclick_menu_data", m_rclickMenuList);
		m_context->setContextProperty("kernel_add_printer_model", m_addPrinterModel);
		m_context->setContextProperty("kernel_parameter_manager", m_parameterManager);
		m_context->setContextProperty("kernel_material_center", m_materialCenter);
		m_context->setContextProperty("kernel_kernelui", m_kernelUI);

		m_engine->setObjectOwnership(m_sliceFlow, QQmlEngine::CppOwnership);
		m_sliceFlow->registerContext();

#if USE_CXCLOUD
		registerImageProvider(QStringLiteral("login_qrcode_image_provider"),
			cxcloud()->getAccountService().lock()->getQrcodeImageProvider());
#endif

		m_kernelUI->loadUserLanguage();
	}

	ModelSpace* Kernel::modelSpace()
	{
		return m_modelSpace;
	}

	CusModelListModel* Kernel::listModel()
	{
		return m_modelListModel;
	}

	ModelNSelector* Kernel::modelSelector()
	{
		return m_modelSelector;
	}

	qtuser_3d::CameraController* Kernel::cameraController()
	{
		return m_cameraController;
	}

	void Kernel::loadUserSettings()
	{
#if USE_CXCLOUD
		cxcloud()->loadUserSettings();
#endif
		setKernelPhase(m_phases[0]);
	}

	int Kernel::currentPhase()
	{
		int index = -1;
		for (int i = 0; i < 3; ++i)
		{
			if (m_currentPhase == m_phases[i])
			{
				index = i;
				break;
			}
		}
		return index;
	}

	ReuseableCache* Kernel::reuseableCache()
	{
		return m_reusableCache;
	}

	RenderCenter* Kernel::renderCenter()
	{
		return m_renderCenter;
	}

	SliceFlow* Kernel::sliceFlow()
	{
		return m_sliceFlow;
	}

	VisualScene* Kernel::visScene()
	{
		return m_visualScene;
	}

	ModelSpaceUndo* Kernel::modelSpaceUndo()
	{
		return m_modelSpaceUndo;
	}

	GlobalConst* Kernel::globalConst()
	{
		return m_globalConst;
	}

	CommandCenter* Kernel::commandCenter()
	{
		return m_commandCenter;
	}

	SensorAnalytics* Kernel::sensorAnalytics()
	{
		return m_sensorAnalytics;
	}

	qtuser_core::ApplicationConfiguration* Kernel::appConfiguration()
	{
		return m_configuration;
	}

	ParameterManager* Kernel::parameterManager()
	{
		return m_parameterManager;
	}

	MaterialCenter* Kernel::materialCenter()
	{
		return m_materialCenter;
	}

	SnapShot* Kernel::snapShot()
	{
		return m_snapShot;
	}

	KernelUI* Kernel::kernelUI()
	{
		return m_kernelUI;
	}

	void Kernel::openFile()
	{
		m_ioManager->open();
	}

	bool Kernel::checkUnsavedChanges()
	{
		return true;
	}

	void Kernel::setGLQuickItem(GLQuickItem* quickItem)
	{
		m_renderCenter->setGLQuickItem(quickItem);
	}

	void Kernel::processCommandLine()
	{
		QStringList params = qApp->arguments();

		if (params.size() > 1)
		{
			QString needOpenName;
			for (const QString& fileName : params)
			{
				QFile file(fileName);
				QFileInfo fileInfo(fileName);
				QString suffix = fileInfo.suffix();
				if (file.exists() && m_ioManager->isSupportSuffix(suffix.toLower()))
				{
					needOpenName = fileName;
					break;
				}
			}

			m_ioManager->openWithName(needOpenName);
		}

		if (params.count() >= 3)
		{
			int count = params.count();
			QString strExcu = params.at(1).toLower();
			if (strExcu == "-e")
			{
				for (int i = 2; i < count; i++)
				{
					QString file = params.at(i);
					QFileInfo info(file);
					QString sufixx = info.suffix();
					if (sufixx.isEmpty())
						continue;
					m_ioManager->openWithName(file);
				}
			}
		}
	}

	void Kernel::setKernelPhase(int phase)
	{
		if (phase >= 0 && phase < 3)
			setKernelPhase(m_phases[phase]);

        if (phase == 0)
        {
            //����ƽ̨�ߴ�
			updatePrinterBox(m_parameterManager->currentMachineWidth()
                , m_parameterManager->currentMachinedepth()
                , m_parameterManager->currentMachineheight());
        }
	}

	void Kernel::setKernelPhase(KernelPhaseType phase)
	{
		QMetaObject::invokeMethod(this, "setKernelPhase", Q_ARG(int, (int)phase));
	}

	void Kernel::viewChanged(bool requestUpdate)
	{
		if (isRenderRenderGraph(m_visualScene))
		{
			m_visualScene->updateRender(requestUpdate);
		}
		else
		{
			renderOneFrame();
		}
	}

	void Kernel::setKernelPhase(KernelPhase* phase)
	{
		if (m_currentPhase == phase)
			return;

		if (m_currentPhase)
		{
			m_currentPhase->onStopPhase();
			m_currentPhase = nullptr;
		}

		m_currentPhase = phase;

		if (m_currentPhase)
			m_currentPhase->onStartPhase();

		emit currentPhaseChanged();
	}

	void Kernel::captureSceneByZproject()
	{
		if (currentPhase() != 0)
		{
			qDebug() << "capture node attach to visualScene";
			return;
		}

		float w = m_parameterManager->currentMachineWidth();
		float d = m_parameterManager->currentMachinedepth();
		float h = m_parameterManager->currentMachineheight();

		m_capture->setResolution(QSize(1000 * w/d, 1000));
		m_capture->attachToGraphNode(m_visualScene->getCameraViewportFrameGraphNode());

		QVector3D max(w, d, h);
		qtuser_3d::Box3D box(QVector3D(0.0, 0.0, 0.0), max);
		m_capture->focusBox(box);

		renderOneFrame();
		m_capture->startCapture();
		//renderOneFrame();
	}
}
