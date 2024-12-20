#include "kernel.h"

#include "data/modelspace.h"
#include "kernel/modelnselector.h"
#include "qtusercore/module/systemutil.h"

#include "qtuser3d/camera/cameracontroller.h"
#include "qtuser3d/module/rendercenter.h"
//#include "qtuser3d/module/glquickitem.h"
#include "qtusercore/util/undoproxy.h"
#include "qtusercore/module/jobexecutor.h"
#include "qtusercore/util/applicationconfiguration.h"
#include "qtusercore/module/cxopenandsavefilemanager.h"
#include <qtusercore/string/resourcesfinder.h>

#include <cxcloud/service_center.h>

#include "kernel/visualscene.h"

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
#include "interface/visualsceneinterface.h"
#include "interface/modelgroupinterface.h"

#include "interface/spaceinterface.h"
#include "cxkernel/interface/iointerface.h"
#include "cxkernel/wrapper/eventtracking.h"
#include "interface/machineinterface.h"
#include "cxkernel/interface/modelninterface.h"
#include "interface/printerinterface.h"
#include "internal/multi_printer/printermanager.h"
#include "internal/layout/layoutmanager.h"

#include "kernel/reuseablecache.h"
#include "kernel/commandcenter.h"
#include "kernel/translator.h"
#include "kernel/snapshot.h"
#include "kernel/kernelui.h"
#include "kernel/preferencemanager.h"
#include "kernel/capturemodeln.h"
#include "kernel/displaymessagemanager.h"

#include "internal/utils/dumpproxy.h"
#include "internal/render/printerentity.h"
#include "internal/utils/themenotifier.h"
#include "internal/data/ObjectsDataManager.h"
#include "internal/data/cusaddprintermodel.h"
#include "internal/rclick_menu/rclickmenulist.h"
#include "internal/parameter/parametermanager.h"
#include "internal/parameter/materialcenter.h"
#include "internal/parameter/uiparametermanager.h"
#include "internal/utils/cadloader.h"
#include "internal/utils/meshloadercenter.h"
#include "interface/projectinterface.h"
#include "localnetworkinterface/devicephase.h"
#include "internal/project_cx3d/cx3dmanager.h"
#include "internal/utils/messagenotify.h"
#include "unittest/unittestflow.h"
#include "interface/unittestinterface.h"
#include "external/data/shareddatamanager/shareddatamanager.h"
#include "external/localnetworkinterface/gcodecolormanager.h"
#include "slice/sliceflow.h"
#include "slice/slicepreviewflow.h"

#include "qtusercore/module/creativeplugincenter.h"
#include "qtusercore/macro.h"
#include "qtuser3d/module/quickscene3dwrapper.h"
//#include "qtuserqml/macro.h"

#include <QtQml/QQmlEngine>
#include <QtQml/QQmlContext>
#include <QtCore/QCoreApplication>
#include <QtCore/QFileInfo>
#include <QtCore/QProcess>

#include "cxffmpeg/qmlplayer.h"
#include "qcxchart.h"
#include <qtusercore/module/quazipfile.h>
#include "crslice/base/parametermeta.h"
#include "crsliceadaptor.h"

#include <qtusercore/util/settings.h>
#include "plugins.h"
#include <QCursor>
#include <cpr/cpr.h>

#define CXSW_SCOPE "com.cxsw.SceneEditor3D"
#define VERSION_MAJOR 1
#define VERSION_MINOR 0
#define COMMON_REASON "Created by Cpp"

#define CXSW_REG(x) QML_INTERFACE(x, CXSW_SCOPE, VERSION_MAJOR, VERSION_MINOR)
#define CXSW_REG_U(x) QML_INTERFACE_U(x, CXSW_SCOPE, VERSION_MAJOR, VERSION_MINOR, COMMON_REASON)

/// USE_CXCLOUD may defined by cxcloud/CMakeList.txt.
/// CUSTOMIZED and CUSTOM_CXCLOUD_ENABLED may defined by customized/module_info.cmake.
/// CXCLOUD_ENABLED defined here when actually using the cxcloud module.
#ifdef USE_CXCLOUD
#  ifdef CUSTOMIZED
#    ifdef CUSTOM_CXCLOUD_ENABLED
#      define CXCLOUD_ENABLED
#  	 endif
#  else
#  endif
#    define CXCLOUD_ENABLED
#endif

// compatibility with resource files of v4.3.
#define OLD_PROJECT_NAME "Creative3D"

namespace creative_kernel
{
	Kernel* gKernel = nullptr;
	std::atomic<bool> gKernelValid{ false };

	Kernel* getKernel()
	{
		return gKernel;
	}

	Kernel* getKernelSafely()
	{
		return gKernelValid ? gKernel : nullptr;
	}

	cxkernel::AppModule* createC3DContext()
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
		gKernelValid = true;

		m_commandCenter = new CommandCenter(this);

		m_configuration = new qtuser_core::ApplicationConfiguration(this);
		m_configuration->registerBundle(global_render_bundle);
		m_configuration->registerBundle(plugin_render_bundle);
		m_configuration->registerBundle(global_render_light_bundle);
		m_configuration->registerBundle(plugin_render_light_bundle);
		m_configuration->registerBundle(default_config_bundle);

		m_modelSpace = new ModelSpace(this);

		m_renderCenter = new qtuser_3d::RenderCenter(this);

		m_cameraController = new qtuser_3d::CameraController(this);
		m_cameraController->setEnableZoomAroundCursor(true);
		m_cameraController->setNeedAroundRotate(true);
		m_preferenceMng = new PreferenceManager(this);
		m_kernelUI = new KernelUI(this);
		m_cx3dManager = new CX3DManager(this);
		m_visualScene = new VisualScene();
		m_unitTest = new UnitTestFlow(this);
		m_sharedDataManager = new SharedDataManager(m_visualScene);

		m_modelSelector = new ModelNSelector(this);
		m_modelSelector->setPickerSource(m_visualScene->facePicker());

		m_reusableCache = new ReuseableCache(m_visualScene);
		connect(m_reusableCache->getPrinterMangager(), &PrinterMangager::signalDidSelectPrinter, m_modelSelector, &ModelNSelector::onSelectedPlateChanged);
		connect(m_reusableCache->getPrinterMangager(), &PrinterMangager::signalDidSelectPrinter, m_modelSpace, &ModelSpace::onSelectedPrinterChanged);

		m_modelSpaceUndo = new ModelSpaceUndo(this);

		m_ObjectsDataManager = new ObjectsDataManager(this);
		m_layoutManager = new LayoutManager(this);

		m_themeNotifier = new ThemeNotifier(this);

		m_dumpProxy = new DumpProxy(this);

		m_sliceFlow = new SliceFlow(this);
		connect(m_reusableCache->getPrinterMangager(), &PrinterMangager::signalPrintersCountChanged, m_sliceFlow, &SliceFlow::onPictureUpdated);
		m_sensorAnalytics = new cxkernel::EventTracking(this);

		m_phases[0] = m_visualScene;
		m_phases[1] = m_sliceFlow;
		m_phases[2] = new DevicePhase(this);

		m_rclickMenuList = new RClickMenuList(this);
		m_parameterManager = new ParameterManager(this);
		ui_parameter_manager_ =
				std::make_unique<UiParameterManager>(m_parameterManager, m_sliceFlow->getEngineType());

		m_snapShot = new SnapShot(this);
		m_cadLoader = new CADLoader(this);
		m_meshLoadCenter = new MeshLoadCenter(this);

		m_capture = new CaptureModelN(this);
		m_capture->setRequestCallback([this](auto) { Q_EMIT scenceVerticalViewUpdated(); });
		m_message_notifier = new MessageNotify(this);

		m_displayMessageManager = new DisplayMessageManager(this);
		connect(m_parameterManager, &ParameterManager::enableSupportChanged, m_displayMessageManager, &DisplayMessageManager::onGlobalEnableSupportChanged);
		connect(m_parameterManager, &ParameterManager::curMachineIndexChanged, m_displayMessageManager, &DisplayMessageManager::updateBedTypeInvalidMessage);
		connect(m_parameterManager, &ParameterManager::currBedTypeChanged, m_displayMessageManager, &DisplayMessageManager::updateBedTypeInvalidMessage);
		connect(m_reusableCache->getPrinterMangager(), &PrinterMangager::signalDidSelectPrinter, m_displayMessageManager, &DisplayMessageManager::updateBedTypeInvalidMessage);

		m_modelSelector->setModelSpace(m_modelSpace);

		m_ObjectsDataManager->setModelSelector(m_modelSelector);
		m_ObjectsDataManager->setModelSpace(m_modelSpace);
		m_ObjectsDataManager->setPrinterManager(m_reusableCache->getPrinterMangager());

		m_modelSpace->setSpaceUndo(m_modelSpaceUndo);
		m_modelSpace->setSharedDataManager(m_sharedDataManager);

		m_modelSpace->addSpaceTracer(m_ObjectsDataManager);
		m_modelSpace->addSpaceTracer(m_displayMessageManager);
		m_reusableCache->getPrinterMangager()->setTracer(m_ObjectsDataManager);

		m_modelSpace->setModelSelector(m_modelSelector);
		m_modelSelector->addModelNSelectorTracer(m_ObjectsDataManager);


		m_gcodeColorMng = new GCodeColorManager(this);

	}

	Kernel::~Kernel()
	{
		gKernelValid = false;
	}

	bool Kernel::existError()
	{
		return m_existError;
	}

	void Kernel::setExistError(bool error)
	{
		m_existError = error;
	}

	QString Kernel::entryQmlFile()
	{
		return QLatin1String("qrc:/scence3d/res/main.qml");
	}

	QStringList Kernel::pluginFilter()
	{
		QStringList list = (QStringLiteral("%1").arg(ACTIVE_PLUGINS)).split(";");
		return list;
	}
	void Kernel::initialize()
	{
		qDebug() << "kernel initialize";
		m_globalConst->initialize();

#ifdef CXCLOUD_ENABLED

		// service center
		cxcloud()->setSslCaInfoFilePath(QFileInfo{
			qtuser_core::getResourcesFolder(QStringLiteral("ssl/usertrust-sectigo.pem"))
		}.absoluteFilePath());
		cxcloud()->setApplicationType(cxcloud::ApplicationType::CREALITY_PRINT);
		cxcloud()->setInterfaceType([]() {
			if (QStringLiteral(PROJECT_VERSION_EXTRA).toLower() != QStringLiteral("alpha")) {
				return cxcloud::InterfaceType::RELEASE;
			} else {
				return cxcloud::InterfaceType::PRE_RELEASE;
			}
		}());

		// model service
		cxcloud()->setSelectedModelCombineSaver([](const QString& dir_path) {
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
		cxcloud()->setSelectedModelUncombineSaver([](const QString& dir_path) {
			QList<QString> file_path_list;
			cmdSaveSelectStl(dir_path, file_path_list);
			for (auto& file_path : file_path_list) {
				file_path = QStringLiteral("%1/%2").arg(dir_path).arg(file_path);
			}
			return file_path_list;
		});

		// slice service
		cxcloud()->setCurrentSliceSaver([this](const QString& dir_path) {
			return m_sliceFlow->saveCurrentGcodeToDir(dir_path);
		});
		cxcloud()->setVaildSliceFileSuffixList({ QStringLiteral("gcode") });
		cxcloud()->setOnlineSliceFileCompressed(true);

		// slice service && project service && download service
		cxcloud()->setOpenFileListHandler(cxkernel::openFileWithNames);
		cxcloud()->setOpenFileHandler(cxkernel::openFileWithName);
		cxcloud()->setOpenProjectHandler([this](const QString& project_path) {
			m_cx3dManager->openProject(project_path, true);
		});

		// model library service && download service
		cxcloud()->setModelCacheDirPath(QFileInfo{
			m_preferenceMng->downloadModelPath()
			//qtuser_core::getOrCreateAppDataLocation(QStringLiteral("../../%1_%2")
			//		.arg(QStringLiteral(OLD_PROJECT_NAME)).arg(QStringLiteral("cloud_models")))
		}.absoluteFilePath());

		// download service
		cxcloud()->setMaxDownloadThreadCount(8);

		// profile service
		auto service = cxcloud()->getProfileService().lock();
		QString engineVersion = QString::fromStdString(CrsliceAdaptor::getInstance().engineVersion());
		m_globalConst->setEngineVersion(engineVersion);
		engineVersion = engineVersion.split("-").size()>1?  engineVersion.split("-")[1]: engineVersion;
		service->setEngineVersion(
				cxcloud::Version{ engineVersion });
		service->setProfileDirPath(
				qtuser_core::getOrCreateAppDataLocation(m_globalConst->getEnginePathPrefix() + "default"));
		service->setUserProfileDirPath(
				qtuser_core::getOrCreateAppDataLocation(m_globalConst->getEnginePathPrefix() + "user"));
		service->setPrinterDirName(QStringLiteral("parampack"));
		service->setPrinterFileName(QStringLiteral("machineList.json"));
		service->setMaterialFileName(QStringLiteral("materialList.json"));
		service->setMaterialTemplateDirName(QStringLiteral("material_templates"));
		service->setProcessTemplateDirName(QStringLiteral("process_templates"));

		// profile service && parameter manager
		connect(service.get(), &cxcloud::ProfileService::checkOfficalProfileFinished,
				m_parameterManager, &ParameterManager::onCheckUpdateableFinished);
		connect(service.get(), &cxcloud::ProfileService::updateOfficalPrinterFinished,
				m_parameterManager, &ParameterManager::onUpdateOfficalPrinterFinished);
		connect(service.get(), &cxcloud::ProfileService::overrideOfficalProfileFinished,
				m_parameterManager, &ParameterManager::onOverrideOfficalProfileFinished);
		connect(service.get(), &cxcloud::ProfileService::updateProfileLanguageFinished,
				m_parameterManager, &ParameterManager::onUpdateProfileLanguageFinished);

		connect(m_preferenceMng, &creative_kernel::PreferenceManager::downPathChanged, this, [=]
			{
				cxcloud()->setModelCacheDirPath(QFileInfo{
						m_preferenceMng->downloadModelPath()
					}.absoluteFilePath());
			});
		// services linkage
		cxcloud()->initialize();
#endif

		m_sliceFlow->initialize();
		m_dumpProxy->initialize();

		m_cameraController->setScreenCamera(m_reusableCache->mainScreenCamera());
		m_cameraController->setMouseSensitivity(2.92);
		m_reusableCache->intialize();
		cxkernel::registerOpenHandler(m_cadLoader);

		connect(m_cameraController, SIGNAL(signalViewChanged(bool)), this, SLOT(viewChanged(bool)));

		m_modelSpace->initialize();
		m_modelSpace->addSpaceTracer(m_reusableCache);
		m_modelSpace->addSpaceTracer(m_reusableCache->getPrinterMangager());
		m_modelSelector->addModelNSelectorTracer(m_reusableCache);
		if (m_phases[2])
		{
			m_modelSpace->addSpaceTracer((creative_kernel::DevicePhase*)(m_phases[2]));
		}

		m_cadLoader->setModelNDataProcessor(m_meshLoadCenter);
		cxkernel::setModelNDataProcessor(m_meshLoadCenter);

		addResizeEventHandler(m_cameraController);
		addRightMouseEventHandler(m_cameraController);
		addMidMouseEventHandler(m_cameraController);
		addWheelEventHandler(m_cameraController);
		addHoverEventHandler(m_cameraController);

		m_cameraController->home(qtuser_3d::triBox2Box3D(getModelSpace()->getBaseBoundingBox()), 1);

		connect(m_visualScene, &VisualScene::sceneChanged, this, &Kernel::visSceneChanged);
		m_visualScene->initialize();

		registerRenderGraph(m_visualScene);
		addUIVisualTracer(m_themeNotifier, m_themeNotifier);

		m_sensorAnalytics->setPath(ANALYTICS_PATH);

		connect(m_parameterManager, &ParameterManager::curMachineIndexChanged, this, &Kernel::onMachineChanged);
		m_parameterManager->initialize();
		connect(m_parameterManager, &ParameterManager::currentColorsChanged, this, &Kernel::onModelMaterialUpdate);
		ui_parameter_manager_->initialize();

		qDebug() << "kernel initialize over";
		showSysMemory();

		m_kernelUI->initialize();

		loadUserSettings();
		m_kernelUI->loadUserSettings();

		m_reusableCache->getPrinterMangager()->initialize();

		setVisOperationMode(NULL);

	    qtuser_core::VersionSettings setting;
        setting.beginGroup("view_show");
        int nRender = setting.value("render_type").toInt();
        // setting.endGroup();
		setModelVisualMode((creative_kernel::ModelVisualMode)nRender);

		qtuser_3d::CameraController* obj = getKernel()->cameraController();
		// setting.beginGroup("view_show");
		int nPerspectiveType = setting.value("perspective_type").toInt();
		if (!nPerspectiveType) {
			setting.setValue("perspective_type", ePerspectiveViewShow);
		}
		setting.endGroup();

		switch (nPerspectiveType)
		{
		case ePerspectiveViewShow:
			obj->viewFromPerspective();
			break;
		case eOrthographicViewShow:
			obj->viewFromOrthographic();
			break;
		}

	}

	void Kernel::uninitialize()
	{
		m_kernelUI->uninitialize();
		m_cx3dManager->uninitialize();
		m_engine->removeImageProvider(QStringLiteral("scence_vertical_view_provider"));

		qDebug() << "kernel uninitialize start";
		m_renderCenter->uninitialize();
		m_modelSpace->uninitialize();
		cxcloud()->uninitialize();
		ui_parameter_manager_->uninitialize();

		renderRenderGraph(nullptr);
		m_reusableCache->blockRelation();
		m_sliceFlow->uninitialize();

		qDebug() << "kernel uninitialize over";
	}

	void Kernel::initializeContext()
	{
		//CXSW_REG(GLQuickItem);
		using namespace qtuser_3d;
    	CXSW_REG(QuickScene3DWrapper);
		CXSW_REG(QMLPlayer);
		CXSW_REG(QCxChart);

		CXSW_REG_U(ToolCommand);
		CXSW_REG_U(ActionCommand);

		m_engine->addImageProvider(QStringLiteral("scence_vertical_view_provider"),
															 m_capture->createImageProvider());

		m_kernelUI->registerQmlEngine(*m_engine);
		m_cx3dManager->initialize();
		m_context->setContextProperty("project_kernel", m_cx3dManager);

		m_context->setContextProperty("kernel_kernel", this);
		m_context->setContextProperty("kernel_project_ui", creative_kernel::projectUI());
		m_context->setContextProperty("kernel_preference_manager", m_preferenceMng);

		m_context->setContextProperty("kernel_global_const", m_globalConst);
		m_context->setContextProperty("kernel_command_center", m_commandCenter);
		m_context->setContextProperty("kernel_render_center", m_renderCenter);
		// m_context->setContextProperty("kernel_render_center_scene3d", m_renderCenter->scene3DWrapper());
		m_context->setContextProperty("kernel_reusable_cache", m_reusableCache);
		m_context->setContextProperty("kernel_modelspace", m_modelSpace);
		m_context->setContextProperty("kernel_mesh_loader_center", m_meshLoadCenter);
		m_context->setContextProperty("kernel_model_selector", m_modelSelector);
		m_context->setContextProperty("kernel_objects_list_data", m_ObjectsDataManager);
		m_context->setContextProperty("kernel_camera_controller", m_cameraController);
		m_context->setContextProperty("kernel_undo_proxy", m_undoProxy);
		m_context->setContextProperty("kernel_io_manager", m_ioManager);
		m_context->setContextProperty("kernel_slice_flow", m_sliceFlow);
		m_context->setContextProperty("kernel_sensor_analytics", m_sensorAnalytics);
		m_context->setContextProperty("kernel_rclick_menu_data", m_rclickMenuList);
		m_context->setContextProperty("kernel_parameter_manager", m_parameterManager);
		m_context->setContextProperty("kernel_add_printer_model", m_parameterManager->getAddPrinterListModelObject());
		m_context->setContextProperty("kernel_material_center", m_parameterManager->getMaterialCenterObject());
		m_context->setContextProperty("kernel_ui_parameter", ui_parameter_manager_.get());
		m_context->setContextProperty("kernel_kernelui", m_kernelUI);
		m_context->setContextProperty("kernel_printer_manager", QVariant::fromValue((QObject*)m_reusableCache->getPrinterMangager()));
		m_context->setContextProperty("kernel_gcodecolor_manager", QVariant::fromValue((QObject*)m_gcodeColorMng));

		connect(m_undoProxy, &qtuser_core::UndoProxy::canUndoChanged, creative_kernel::projectUI(), &ProjectInfoUI::spaceDirtyChanged);

        m_context->setContextProperty("kernel_vld_enabled", QVariant::fromValue(false));
#ifdef _WIN32
	#if( (defined CXX_CHECK_MEMORY_LEAKS) && (defined _DEBUG) && (_MSC_VER))
        m_context->setContextProperty("kernel_vld_enabled", QVariant::fromValue(true));
	#endif
#endif


		m_engine->setObjectOwnership(m_sliceFlow, QQmlEngine::CppOwnership);
		m_sliceFlow->registerContext();

		m_unitTest->registerContext();
		m_kernelUI->loadUserLanguage();
	}

	ModelSpace* Kernel::modelSpace()
	{
		return m_modelSpace;
	}

	ModelNSelector* Kernel::modelSelector()
	{
		return m_modelSelector;
	}

	std::vector<std::pair<std::string, std::string>> Kernel::modelMetaData() const
	{
		return m_model_metadata;
	}

	void Kernel::setModelMetaData(const std::vector<std::pair<std::string, std::string>>& data)
	{
		m_model_metadata = data;
	}

	qtuser_3d::CameraController* Kernel::cameraController()
	{
		return m_cameraController;
	}

	void Kernel::loadUserSettings()
	{
#ifdef CXCLOUD_ENABLED
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

	bool Kernel::isDefaultVisScene()
	{
		return m_visualScene->isDefaultScene();
	}

	ReuseableCache* Kernel::reuseableCache()
	{
		return m_reusableCache;
	}

	qtuser_3d::RenderCenter* Kernel::renderCenter()
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

	cxkernel::EventTracking* Kernel::sensorAnalytics()
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

	UiParameterManager* Kernel::uiParameterManager() {
		return ui_parameter_manager_.get();
	}

	MaterialCenter* Kernel::materialCenter()
	{
		return m_parameterManager->getMaterialCenter();
	}

	SnapShot* Kernel::snapShot()
	{
		return m_snapShot;
	}

	MessageNotify* Kernel::messageNotify()
	{
		return m_message_notifier;
	}

	CX3DManager* Kernel::cx3dManager()
	{
		return m_cx3dManager;
	}
	UnitTestFlow* Kernel::unitTest()
	{
		return m_unitTest;
	}
	SharedDataManager* Kernel::sharedDataManager()
	{
		return m_sharedDataManager;
	}

	RClickMenuList* Kernel::rclickMenuList()
	{
		return m_rclickMenuList;
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
		if(creative_kernel::projectSpaceDirty())
		{
				m_kernelUI->beforeCloseWindow();
				return true;
		}

		bool bModifyed = m_parameterManager->isAnyModified();
		if(bModifyed)
		{
			QMetaObject::invokeMethod(getUIAppWindow(), "saveParemDialog");
		}
		return bModifyed;
	}

	/*void Kernel::setGLQuickItem(GLQuickItem* quickItem)
	{
		m_renderCenter->setGLQuickItem(quickItem);
	}*/

	void Kernel::setScene3DWrapper(qtuser_3d::QuickScene3DWrapper* scene3DItem)
	{
		m_renderCenter->setScene3DWrapper(scene3DItem);
	}

	void Kernel::processCommandLine()
	{
		QStringList params = qApp->arguments();

		qDebug() << "processCommandLine  params" << params.size();
		qDebug() << "processCommandLine  params" << params;
		if (params.size() <= 1) return;
		if (params.size() == 2)
		{
			if (params[1] == "-runtest")
			{
#ifdef _WIN32
				_sleep(10000);
#endif // WIN32 || WIN64
				QCoreApplication::quit();

				return;
			}
			QString needOpenName = "";
			QString curSuffix = "";
			for (const QString& fileName : params)
			{
				QFile file(fileName);
				QFileInfo fileInfo(fileName);
				QString suffix = fileInfo.suffix();
				if (file.exists() && m_ioManager->isSupportSuffix(suffix.toLower()))
				{
					needOpenName = fileName;
					curSuffix = suffix;
					break;
				}
			}
			auto param = params[1];
			if (param.startsWith("crealityprintlink"))
			{
				QString url_3mf = param.mid(param.indexOf("=")+1);
				QString name_3mf = "model.3mf";
				const auto profile_service = cxcloud()->getProfileService().lock();
				if (profile_service)
				{
					QTemporaryDir temp_dir;
					auto file_3mf = temp_dir.filePath(name_3mf);
					auto request = profile_service->createRequest<cxcloud::DownloadRequest>(cpr::util::urlDecode(url_3mf.toStdString()).c_str(), file_3mf);
					request->syncDownload();
					auto server_file_name = request->getServerFileName();
					if (!server_file_name.isEmpty())
					{
						QString file_3mf_new;
						auto filename = QFileInfo(server_file_name).completeBaseName();
						auto suffix = QFileInfo(server_file_name).suffix();
						int i = 0;
						while (true)
						{
							if (i == 0)
							{
								file_3mf_new = QString("%1/%2.%3").arg(QStandardPaths::writableLocation(QStandardPaths::DownloadLocation)).arg(filename).arg(suffix);
							}
							else
							{
								file_3mf_new = QString("%1/%2(%4).%3").arg(QStandardPaths::writableLocation(QStandardPaths::DownloadLocation)).arg(filename).arg(suffix).arg(i);
							}
							if (!QFile::exists(file_3mf_new))
							{
								break;
							}
							++i;
						}
						QFile::copy(file_3mf, file_3mf_new);
						m_cx3dManager->openProject(file_3mf_new);
					}
					return;
				}
			}
			qDebug() << "needOpenName =" << needOpenName;
			if (curSuffix.toLower() == "cxprj" ||
				curSuffix.toLower() == "3mf")
			{
				m_cx3dManager->openProject(needOpenName);   //Do not continue if there is a project file
				return;
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
					if (sufixx.toLower() == "cxprj" ||
						sufixx.toLower() == "3mf")
					{
						m_cx3dManager->openProject(file);   //Do not continue if there is a project file
						return;
					}
					m_ioManager->openWithName(file);
				}
			}
			else if(strExcu == "-bltest")
			{
				QString cmdTxt = "baseline cmd is -->( .exe -bltest -compare  --blpath {blpath} --cmppath {cmppath}) --scppath {scppath}";
				//"baseline cmd is -->( .exe -bltest --3mfpath {3mfpath} --blpath {blpath} --cmppath {cmppath}) -compare ";
				bool cmpBlTestEnabled = true;
				if (params.size() < 9)
				{
					qDebug() << cmdTxt;
					return;
				}
				QString funcCmd = params.at(2);
				if (funcCmd == "-generate")
				{
					BLCompareTestFlow::setFuncType(0);
				}
				else if (funcCmd == "-update")
				{
					BLCompareTestFlow::setFuncType(2);
				}
				else
				{
					BLCompareTestFlow::setFuncType(1);
				}

				QString blpathcmd = params.at(3);
				if (blpathcmd == "--blpath")
				{
					QString mblpath = params.at(4);
					BLCompareTestFlow::setBLPath(mblpath);
				}
				else { cmpBlTestEnabled = false; }
				QString cmpPathcmd = params.at(5);
				if(cmpPathcmd == "--cmppath")
				{
					QString comppath = params.at(6);
					BLCompareTestFlow::setCompareResultPath(comppath);
				}
				else { cmpBlTestEnabled = false; }
				QString mpathcmd = params.at(7);
				if (mpathcmd == "--scppath")
				{
					QString mpath = params.at(8);
					BLCompareTestFlow::set3mfPath(mpath);
				}
				else if (mpathcmd == "--filename")
				{
					QString mpath = params.at(8);
					BLCompareTestFlow::set3mfPath(mpath);
				}
				else if (mpathcmd == "--scplist")
				{
					QStringList listpath;
					for (int i = 8; i < params.size(); i++)
					{
						QString file = params.at(i);
						listpath.push_back(file);
					}
					BLCompareTestFlow::setScpList(listpath);
				}
				else { cmpBlTestEnabled = false; }


				if (!cmpBlTestEnabled)
				{
					qDebug() << cmdTxt;
					return;
				}

				BLCompareTestFlow::setMessageEnabled(false);
				BLCompareTestFlow::setEnabled(true);
				BLCompareTestFlow::startBLTest();

			}
			else if (strExcu == "-slicecache")
			{
				int count = params.count();
				if (count == 6)
				{
					int inputType =  params.at(2).toInt();
					QString _3mfFile = params.at(3);
					QString cacheDir = params.at(4);
					QString gcodeDir = params.at(5);
					UnitTestFlow* unitFLow = unitTestFlow();
					unitFLow->setSliceCachePath(cacheDir);
					unitFLow->setGcodePath(gcodeDir);
					if (inputType == 0)
					{
						unitFLow->sliceGcodeFrom3mf(_3mfFile);
					}
					else if (inputType == 1)
					{
						unitFLow->sliceGcodeFromCache(_3mfFile);
					}


				}
			}
		}

	}
	bool Kernel::blTestEnabled()
	{
		return BLCompareTestFlow::enabled();
	}
	void Kernel::setKernelPhase(int phase)
	{
		if (phase >= 0 && phase < 3)
			setKernelPhase(m_phases[phase]);
	}

	void Kernel::setKernelPhase(KernelPhaseType phase)
	{
		QMetaObject::invokeMethod(this, "setKernelPhase", Q_ARG(int, (int)phase));


	}

	void Kernel::restartApplication() {
		restartApplication(true);
	}

	void Kernel::restartApplication(bool keep_arguments) {
		auto arguments = QCoreApplication::arguments();
		const auto program = std::move(arguments.front());
		keep_arguments ? arguments.pop_front() : arguments.clear();
		QCoreApplication::quit();
		QProcess::startDetached(program, arguments);
	}

	void Kernel::setRightClickType(ClickType type)
	{
		m_rclickMenuList->setMenuType(type);
	}
	bool Kernel::canUndo()
	{
		return m_undoProxy->canUndo();
	}
	void Kernel::viewChanged(bool requestUpdate)
	{
		if (!isUsingPreviewScene())
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

		requestVisPickUpdate(true);
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

	bool Kernel::addPrinter()
	{
		return appendPrinter(true);
	}

	bool Kernel::isMaterialUsed(int materialIndex)
	{
		QList<ModelN*> models = modelns();
		for (auto m : models)
		{
			if (m->isContainsMaterial(materialIndex))
			{
				return true;
			}
		}
		return false;
	}

	void Kernel::selectModelByMaterial(int materialIndex)
	{
		QList<qtuser_3d::Pickable*> selectedModels;
		QList<ModelN*> models = modelns();
		for (auto m : models)
		{
			if (m->isContainsMaterial(materialIndex))
			{
				selectedModels << m;
			}
		}
	}

	void Kernel::discardMaterial(int materialIndex)
	{
		m_sharedDataManager->discardColorIndex(materialIndex);
	}

	void Kernel::changeMaterial(int materialIndex)
	{
		QList<ModelGroup*> groups = selectedGroups();
		if (groups.isEmpty())
		{
			ModelSpaceUndo* stack = modelSpaceUndo();
			stack->changeMaterials(selectionms(), materialIndex);
		}
		else
		{
			setModelGroupsColor(groups, materialIndex, true);
		}

		updateWipeTowers();
	}

	void Kernel::setSceneClearColor(const QColor& color)
	{
		if (m_visualScene)
		{
			m_visualScene->setSceneClearColor(color);
		}
	}

	LayoutManager* Kernel::layoutManager()
	{
		return m_layoutManager;
	}

	DisplayMessageManager* creative_kernel::Kernel::displayMessageManager()
	{
		return m_displayMessageManager;
	}

	void Kernel::onMachineChanged()
	{
		/* remove material */
		std::vector<trimesh::vec> colors = currentColors();
		int count = colors.size();
		m_sharedDataManager->discardColorIndexMoreThan(count);
	}

	void Kernel::onModelMaterialUpdate()
	{
		std::vector<trimesh::vec> colors = currentColors();
		int count = colors.size();
		m_sharedDataManager->discardColorIndexMoreThan(count);
		m_sharedDataManager->updateRenderData(false);
	}

}
