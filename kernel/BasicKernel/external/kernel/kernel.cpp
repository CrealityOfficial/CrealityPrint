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
#include "cxkernel/interface/iointerface.h"
#include "cxkernel/wrapper/eventtracking.h"
#include "interface/machineinterface.h"
#include "cxkernel/interface/modelninterface.h"
#include "interface/printerinterface.h"
#include "internal/multi_printer/printermanager.h"
#include "interface/modelinterface.h"

#include "kernel/reuseablecache.h"
#include "kernel/commandcenter.h"
#include "kernel/translator.h"
#include "kernel/snapshot.h"
#include "kernel/kernelui.h"
#include "kernel/preferencemanager.h"
#include "kernel/capturemodeln.h"

#include "internal/utils/dumpproxy.h"
#include "internal/render/printerentity.h"
#include "internal/utils/printernotifier.h"
#include "internal/utils/themenotifier.h"
#include "internal/data/cusModelListModel.h"
#include "internal/data/platesDataManager.h"
#include "internal/data/cusaddprintermodel.h"
#include "internal/rclick_menu/rclickmenulist.h"
#include "internal/parameter/parametermanager.h"
#include "internal/parameter/materialcenter.h"
#include "internal/parameter/uiparametermanager.h"
#include "internal/utils/cadloader.h"
#include "internal/undo/modelmaterialcommand.h"
#include "interface/projectinterface.h"
#include "localnetworkinterface/devicephase.h"
#include "internal/project_cx3d/cx3dmanager.h"
#include "internal/utils/messagenotify.h"
#include "unittest/unittestflow.h"
#include "utils/namedeclare.h"

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
	Kernel* getKernel()
	{
		return gKernel;
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
		m_modelSelector = new ModelNSelector(this);
		m_modelSelector->setPickerSource(m_visualScene->facePicker());

		m_modelSpaceUndo = new ModelSpaceUndo(this);

		m_reusableCache = new ReuseableCache(m_visualScene);
		m_modelListModel = new CusModelListModel(this);
		m_platesDataManager = new PlatesDataManager(this);
		m_platesDataManager->registerModelList(m_modelListModel);

		m_printerNotifier = new PrinterNotifier(this);
		m_themeNotifier = new ThemeNotifier(this);

		m_dumpProxy = new DumpProxy(this);

		m_sliceFlow = new SliceFlow(this);
		m_sensorAnalytics = new cxkernel::EventTracking(this);

		m_phases[0] = m_visualScene;
		m_phases[1] = m_sliceFlow;
		m_phases[2] = new DevicePhase(this);

		m_rclickMenuList = new RClickMenuList(this);
		m_parameterManager = new ParameterManager(this);
		ui_parameter_manager_ =
				std::make_unique<UiParameterManager>(m_parameterManager, m_sliceFlow->getEngineType());

		connect(m_parameterManager, &ParameterManager::currentColorsChanged, m_modelListModel, &CusModelListModel::onChangeColors);

		m_snapShot = new SnapShot(this);
		m_cadLoader = new CADLoader(this);
		m_cadLoader->setModelNDataProcessor(m_modelSpace);

		m_capture = new CaptureModelN(this);
		m_capture->setRequestCallback([this](auto) { Q_EMIT scenceVerticalViewUpdated(); });
		m_message_notifier = new MessageNotify(this);
	}

	Kernel::~Kernel()
	{
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

		// slice service && download service
		cxcloud()->setOpenFileHandler(cxkernel::openFileWithName);
		cxcloud()->setOpenFileListHandler(cxkernel::openFileWithNames);

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
		m_modelSpace->addSpaceTracer(m_printerNotifier);
		m_modelSpace->addSpaceTracer(m_reusableCache->getPrinterMangager());
		m_modelSelector->addTracer(m_modelListModel);
		m_modelSelector->addTracer(m_printerNotifier);
		if (m_phases[2])
		{
			m_modelSpace->addSpaceTracer((creative_kernel::DevicePhase*)(m_phases[2]));
		}


		m_reusableCache->mainScreenCamera()->addCameraObserver(m_printerNotifier);

		addResizeEventHandler(m_cameraController);
		addRightMouseEventHandler(m_cameraController);
		addMidMouseEventHandler(m_cameraController);
		addWheelEventHandler(m_cameraController);
		addHoverEventHandler(m_cameraController);

		qtuser_3d::Box3D baseBox = getModelSpace()->baseBoundingBox();
		m_cameraController->home(baseBox, 1);

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
		qtuser_3d::Box3D box = getKernel()->modelSpace()->baseBoundingBox();
		QVector3D boxCenter = box.center();
		boxCenter.setZ(0);
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

		cxkernel::setModelNDataProcessor(m_modelSpace);

		m_context->setContextProperty("kernel_kernel", this);
		m_context->setContextProperty("kernel_project_ui", creative_kernel::projectUI());
		m_context->setContextProperty("kernel_preference_manager", m_preferenceMng);

		m_context->setContextProperty("kernel_global_const", m_globalConst);
		m_context->setContextProperty("kernel_command_center", m_commandCenter);
		m_context->setContextProperty("kernel_render_center", m_renderCenter);
		// m_context->setContextProperty("kernel_render_center_scene3d", m_renderCenter->scene3DWrapper());
		m_context->setContextProperty("kernel_reusable_cache", m_reusableCache);
		m_context->setContextProperty("kernel_modelspace", m_modelSpace);
		m_context->setContextProperty("kernel_model_selector", m_modelSelector);
		m_context->setContextProperty("kernel_model_list_data", m_modelListModel);
		m_context->setContextProperty("kernel_plates_list_data", m_platesDataManager);
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




		m_engine->setObjectOwnership(m_sliceFlow, QQmlEngine::CppOwnership);
		m_sliceFlow->registerContext();

		m_unitTest->registerContext();
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
				QString cmdTxt = "baseline cmd is -->( .exe -bltest --3mfpath {3mfpath} --blpath {blpath} --cmppath {cmppath}) -compare ";
				bool cmpBlTestEnabled = true;
				if (params.size() < 9)
				{
					qDebug() << cmdTxt;
					return;
				}
				QString mpathcmd = params.at(2);
				if (mpathcmd == "--3mfpath")
				{
					QString mpath = params.at(3);
					BLCompareTestFlow::set3mfPath(mpath);
				}
				else { cmpBlTestEnabled = false; }

				QString blpathcmd = params.at(4);
				if (blpathcmd == "--blpath")
				{
					QString mblpath = params.at(5);
					BLCompareTestFlow::setBLPath(mblpath);
				}
				else { cmpBlTestEnabled = false; }
				QString cmpPathcmd = params.at(6);
				if(cmpPathcmd == "--cmppath")
				{
					QString comppath = params.at(7);
					BLCompareTestFlow::setCompareResultPath(comppath);
				}
				else { cmpBlTestEnabled = false; }
				QString funcCmd = params.at(8);
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
				if (!cmpBlTestEnabled)
				{
					qDebug() << cmdTxt;
					return;
				}
				BLCompareTestFlow::setEnabled(true);
				BLCompareTestFlow::startBLTest();

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

		QElapsedTimer timer;
		timer.start();
		while (timer.elapsed() < 200)
		{
            QThread::msleep(40);
			QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
			renderOneFrame();
		}
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
		unselectAll();
		selectMore(selectedModels);
	}

	void Kernel::discardMaterial(int materialIndex)
	{
		QSet<ModelNRenderData*> cache;
		QList<ModelN*> ms = modelns();
		for (auto m : ms)
		{
			auto renderData = m->renderData();
			if (cache.contains(renderData.get()))
			{
				m->setRenderData(renderData);
				m->colorsDataChanged();
			}
			else
			{
				renderData->discardMaterial(materialIndex);
				m->updateRender();
				m->colorsDataChanged();
				cache.insert(renderData.get());
			}
		}
	}

	void Kernel::changeMaterial(int materialIndex)
	{
		ModelSpaceUndo* stack = modelSpaceUndo();
		auto ms = selectionms();
		stack->push(new ModelMaterialCommand(ms, materialIndex));
		updateWipeTowers();
	}

	void Kernel::setSceneClearColor(const QColor& color)
	{
		if (m_visualScene)
		{
			m_visualScene->setSceneClearColor(color);
		}

		if (m_sliceFlow)
		{
			m_sliceFlow->slicePreviewFlow()->setSceneClearColor(color);
		}
	}

	void Kernel::onMachineChanged()
	{
		/* remove material */
		std::vector<trimesh::vec> colors = currentColors();
		int count = colors.size();
		QList<int> materialCache;
		for (int i = count; i < 16; ++i)
		{
			if (isMaterialUsed(i))
			{
				materialCache << i;
			}
		}

		if (materialCache.isEmpty())
			return;

		QSet<ModelNRenderData*> cache;
		QList<ModelN*> ms = modelns();
		for (auto m : ms)
		{
			auto renderData = m->renderData();
			if (cache.contains(renderData.get()))
			{
				m->setRenderData(renderData);
			}
			else
			{
				for (int index : materialCache)
				{
					renderData->discardMaterial(index);
				}
				m->updateRender();
				m->colorsDataChanged();
				cache.insert(renderData.get());
			}
		}
	}

	void Kernel::onModelMaterialUpdate()
	{
		QMap<ModelNRenderData*, bool> cache;
		QList<ModelN*> ms = modelns();
		for (auto m : ms)
		{
			auto renderData = m->renderData();
			cache[renderData.get()] = renderData->checkMaterialChanged() != -1;
		}

		auto it = cache.begin(), end = cache.end();
		for (; it != end; ++it)
		{
			if (it.value())
				it.key()->updateRenderDataForced();
		}

		for (auto m : ms)
		{
			auto renderData = m->renderData();
			if (cache[renderData.get()])
			{
				m->setRenderData(renderData);
				m->dirty();
			}
		}
	}
}
