#include "cxcloud/service_center.h"

#include <QtCore/QLocale>

#include "cxcloud/define.h"
#include "cxcloud/tool/function.h"
#include "cxcloud/tool/settings.h"

namespace cxcloud {

  namespace {

    inline auto ServerTypeToString(ServerType type) -> QString {
      return QString::number(static_cast<int>(type));
    }

  }  // namespace

  ServiceCenter::ServiceCenter(QObject* parent)
      : ServiceCenter{ { 0, 0, 0, 0 }, parent } {}

  ServiceCenter::ServiceCenter(Version local_version, QObject* parent)
      : QObject{ parent }
      , server_list_model_{ std::make_unique<ServerListModel>(ServerListModel::DataContainer{
          { ServerTypeToString(ServerType::MAINLAND_CHINA), QStringLiteral("China")         },
          { ServerTypeToString(ServerType::ASIA_PASCIFIC) , QStringLiteral("Asia-Pacific") },
          { ServerTypeToString(ServerType::EUROPE)        , QStringLiteral("Europe")        },
          { ServerTypeToString(ServerType::NORTH_AMERICA) , QStringLiteral("North America") },
          { ServerTypeToString(ServerType::SOUTH_AMERICA) , QStringLiteral("South America") },
          { ServerTypeToString(ServerType::OTHERS)        , QStringLiteral("Others")        },
        }) }
      , http_session_{ std::make_shared<HttpSession>(local_version,
                                                     [this] { return getApiUrl(); }) }
      , job_executor_{ std::make_shared<JobExecutor>() }
      , account_service_{ std::make_shared<AccountService>(http_session_) }
      , model_service_{ std::make_shared<ModelService>(http_session_) }
      , slice_service_{ std::make_shared<SliceService>(http_session_) }
      , project_service_{ std::make_shared<ProjectService>(http_session_, job_executor_) }
      , printer_service_{ std::make_shared<PrinterService>(http_session_) }
      , model_library_service_{ std::make_shared<ModelLibraryService>(http_session_) }
      , download_service_{ std::make_shared<DownloadService>(http_session_) }
      , upgrade_service_{ std::make_shared<UpgradeService>(http_session_,
                                                           std::move(local_version)) }
      , profile_service_{ std::make_shared<ProfileService>(http_session_) } {
    // register account uid getter and account token getter after account service created.
    http_session_->setAccountUidGetter([this] { return account_service_->getUserId(); });
    http_session_->setAccountTokenGetter([this] { return account_service_->getToken(); });
  }

  auto ServiceCenter::initialize() -> void {
    model_service_->setUserIdGetter([this] { return account_service_->getUserId(); });

    slice_service_->setWebUrlGetter([this] { return getWebUrl(); });

    printer_service_->setServerTypeGetter([this] { return getServerType(); });

    model_library_service_->setServerTypeGetter([this] { return getServerType(); });
    model_library_service_->setModelGroupUrlCreater([this](const QString& group_uid) {
      return QStringLiteral("%1%2").arg(getModelGroupUrlHead()).arg(group_uid);
    });

    connect(this, &ServiceCenter::serverTypeChanged,
            model_library_service_.get(), &ModelLibraryService::onServerTypeChanged);

    connect(account_service_.get(), &AccountService::loginedChanged, this, [this] {
      setRestrictedTipEnabled(true);
      updateModelRestriction();
    });

    connect(account_service_.get(), &AccountService::loginSuccessed,
            download_service_.get(), &DownloadService::fulfillsAllDonwloadPromise);

    connect(project_service_.get(), &ProjectService::uploadedProjectDeleted,
            model_service_.get(), &ModelService::onProjectDeleted);

    connect(model_library_service_.get(), &ModelLibraryService::modelGroupUncollected,
            model_service_.get(), &ModelService::onModelGroupUncollected);
  }

  auto ServiceCenter::uninitialize() -> void {
    account_service_->uninitialize();
    slice_service_->uninitialize();
    model_service_->uninitialize();
    project_service_->uninitialize();
    printer_service_->uninitialize();
    model_library_service_->uninitialize();
    download_service_->uninitialize();
    upgrade_service_->uninitialize();
    profile_service_->uninitialize();
  }

  auto ServiceCenter::loadUserSettings() -> void {
    account_service_->initialize();
    slice_service_->initialize();
    model_service_->initialize();
    project_service_->initialize();
    printer_service_->initialize();
    model_library_service_->initialize();
    download_service_->initialize();
    upgrade_service_->initialize();
    profile_service_->initialize();
  }

  auto ServiceCenter::setOpenFileHandler(std::function<void(const QString&)> handler) -> void {
    open_file_handler_ = handler;
    slice_service_->setOpenFileHandler(handler);
    download_service_->setOpenFileHandler(handler);
  }

  auto ServiceCenter::setOpenFileListHandler(
      std::function<void(const QStringList&)> handler) -> void {
    open_file_list_handler_ = handler;
    download_service_->setOpenFileListHandler(handler);
  }

  auto ServiceCenter::setOpenProjectHandler(std::function<void(const QString&)> handler) -> void {
    open_project_handler_ = handler;
    project_service_->setOpenFileHandler(handler);
  }

  auto ServiceCenter::setSelectedModelCombineSaver(
      std::function<QString(const QString&)> saver) -> void {
    selected_model_combine_saver_ = saver;
    model_service_->setSelectedModelCombineSaver(saver);
  }

  auto ServiceCenter::setSelectedModelUncombineSaver(
      std::function<QStringList(const QString&)> saver) -> void {
    selected_model_uncombine_saver_ = saver;
    model_service_->setSelectedModelUncombineSaver(saver);
  }

  auto ServiceCenter::setCurrentSliceSaver(std::function<QString(const QString&)> saver) -> void {
    current_slice_saver_ = saver;
    slice_service_->setCurrentSliceSaver(saver);
  }

  auto ServiceCenter::getVaildSliceFileSuffixList() const -> QStringList {
    return vaild_slice_file_suffix_list_;
  }

  auto ServiceCenter::setVaildSliceFileSuffixList(const QStringList& list) -> void {
    vaild_slice_file_suffix_list_ = list;
    slice_service_->setVaildSuffixList(list);
  }

  auto ServiceCenter::isOnlineSliceFileCompressed() const -> bool {
    return online_slice_file_compressed_;
  }

  auto ServiceCenter::setOnlineSliceFileCompressed(bool compressed) -> void {
    online_slice_file_compressed_ = compressed;
    slice_service_->setOnlineSliceFileCompressed(compressed);
  }

  auto ServiceCenter::getModelCacheDirPath() const -> QString {
    return model_cache_dir_path_;
  }

  auto ServiceCenter::setModelCacheDirPath(const QString& path) -> void {
    model_cache_dir_path_ = path;
    model_library_service_->setCacheDirPath(path);
    download_service_->setCacheDirPath(path);
  }

  auto ServiceCenter::getMaxDownloadThreadCount() const -> size_t {
    return max_download_thread_count_;
  }

  auto ServiceCenter::setMaxDownloadThreadCount(size_t count) -> void {
    max_download_thread_count_ = count;
    download_service_->setMaxDownloadThreadCount(count);
  }

  auto ServiceCenter::getSslCaInfoFilePath() const -> QString {
    return http_session_->getSslCaInfoFilePath();
  }

  auto ServiceCenter::setSslCaInfoFilePath(const QString& path) -> void {
    http_session_->setSslCaInfoFilePath(path);
  }

  auto ServiceCenter::getJsEngine() const -> QPointer<QJSEngine> {
    return js_engine_;
  }

  auto ServiceCenter::setJsEngine(QPointer<QJSEngine> engine) -> void {
    js_engine_ = engine;

    account_service_->setJsEngine(engine);
    model_service_->setJsEngine(engine);
    slice_service_->setJsEngine(engine);
    project_service_->setJsEngine(engine);
    printer_service_->setJsEngine(engine);
    model_library_service_->setJsEngine(engine);
    download_service_->setJsEngine(engine);
    upgrade_service_->setJsEngine(engine);
    profile_service_->setJsEngine(engine);
  }

// ----------- base [begin] -----------

  auto ServiceCenter::getApplicationType() const -> ApplicationType {
    return http_session_->getApplicationType();
  }

  auto ServiceCenter::setApplicationType(ApplicationType type) -> void {
    http_session_->setApplicationType(type);
  }

  auto ServiceCenter::getInterfaceType() const -> InterfaceType {
    return interface_type_;
  }

  auto ServiceCenter::setInterfaceType(InterfaceType type) -> void {
    interface_type_ = type;
  }

  auto ServiceCenter::getServerType() const -> ServerType {
    return CloudSettings{}.getServerType();
  }

  auto ServiceCenter::setServerType(ServerType new_type) -> void {
    auto settings = CloudSettings{};

    auto old_type = settings.getServerType();

    settings.setServerType(new_type);

    if (old_type != new_type) {
      serverTypeChanged();
    }

    if (ServerTypeToRealServerType(old_type) != ServerTypeToRealServerType(new_type)) {
      realServerTypeChanged();
    }
  }

  auto ServiceCenter::getServerTypeIndex() const -> int {
    return static_cast<int>(getServerType());
  }

  auto ServiceCenter::setServerTypeIndex(int type_index) -> void {
    setServerType(static_cast<ServerType>(type_index));
  }

  auto ServiceCenter::getRealServerType() const -> RealServerType {
    return ServerTypeToRealServerType(getServerType());
  }

  auto ServiceCenter::getRealServerTypeIndex() const -> int {
    return static_cast<int>(getRealServerType());
  }

  auto ServiceCenter::toRealServerType(int server_type_index) const -> int {
    auto server_type = static_cast<ServerType>(server_type_index);
    auto real_server_type = ServerTypeToRealServerType(server_type);
    return static_cast<int>(real_server_type);
  }

  auto ServiceCenter::toServerType(int real_server_type_index) const -> int {
    auto real_server_type = static_cast<RealServerType>(real_server_type_index);
    auto server_type = RealServerTypeToServerType(real_server_type);
    return static_cast<int>(server_type);
  }

  auto ServiceCenter::getServerList() const -> ServerListModel::DataContainer {
    return server_list_model_->rawData();
  }

  auto ServiceCenter::setServerList(ServerListModel::DataContainer list) -> void {
    server_list_model_->clear();
    server_list_model_->pushBack(std::move(list));
  }

  auto ServiceCenter::getServerListModel() const -> QAbstractListModel* {
    return server_list_model_.get();
  }

  auto ServiceCenter::getApiUrl() const -> QString {
    switch (getInterfaceType()) {
      case InterfaceType::DEVELOP: {
        return QStringLiteral("http://api-dev.crealitycloud.cn");
      }
      case InterfaceType::PRE_RELEASE: {
        switch (getRealServerType()) {
          case RealServerType::MAINLAND_CHINA: {
            return QStringLiteral("https://admin-pre.crealitycloud.cn");
          }
          default: {
            return QStringLiteral("https://admin-pre.crealitycloud.com");
          }
        }
      }
      case InterfaceType::RELEASE:
      default: {
        switch (getRealServerType()) {
          case RealServerType::MAINLAND_CHINA: {
            return QStringLiteral("https://api.crealitycloud.cn");
          }
          default: {
            return QStringLiteral("https://api.crealitycloud.com");
          }
        }
      }
    }
  }

  auto ServiceCenter::getWebUrl() const -> QString {
    switch (getInterfaceType()) {
      case InterfaceType::PRE_RELEASE: {
        switch (getRealServerType()) {
          case RealServerType::MAINLAND_CHINA: {
            return QStringLiteral("https://pre.crealitycloud.cn");
          }
          default: {
            return QStringLiteral("https://pre.crealitycloud.com");
          }
        }
      }
      case InterfaceType::RELEASE:
      default: {
        switch (getRealServerType()) {
          case RealServerType::MAINLAND_CHINA: {
            return QStringLiteral("https://www.crealitycloud.cn");
          }
          default: {
            return QStringLiteral("https://www.crealitycloud.com");
          }
        }
      }
    }
  }

  auto ServiceCenter::getModelGroupUrlHead() const -> QString {
    return QStringLiteral("%1/model-detail/").arg(getWebUrl());
  }

  auto ServiceCenter::loadModelGroupUrl(const QString& group_uid) -> QString {
    return QStringLiteral("%1%2").arg(getModelGroupUrlHead()).arg(group_uid);
  }

  auto ServiceCenter::getModelRestriction() const -> ModelRestriction {
    return http_session_->getModelRestriction();
  }

  auto ServiceCenter::setModelRestriction(ModelRestriction restriction) -> void {
    if (http_session_->getModelRestriction() != restriction) {
      http_session_->setModelRestriction(restriction);
      modelRestrictionChanged();
    }
  }

  auto ServiceCenter::getModelRestrictionString() const -> QString {
    return ModelRestrictionToString(getModelRestriction());
  }

  auto ServiceCenter::setModelRestrictionString(const QString& restriction) -> void {
    setModelRestriction(StringToModelRestriction(restriction));
  }

  auto ServiceCenter::isRestrictedTipEnabled() const -> bool {
    return restricted_tip_enabled_;
  }

  auto ServiceCenter::setRestrictedTipEnabled(bool enabled) -> void {
    if (restricted_tip_enabled_ != enabled) {
      restricted_tip_enabled_ = enabled;
      restrictedTipEnabledChanged();
    }
  }

  auto ServiceCenter::getJobExecutor() const -> std::weak_ptr<JobExecutor> {
    return job_executor_;
  }

  auto ServiceCenter::getJobExecutorObject() const -> QObject* {
    return job_executor_.get();
  }

// ----------- base [end] -----------

// ----------- sub servive [begin] -----------

  auto ServiceCenter::getAccountService() const -> std::weak_ptr<AccountService> {
    return account_service_;
  }

  auto ServiceCenter::getAccountServiceObject() const -> QObject* {
    return account_service_.get();
  }

  auto ServiceCenter::getModelService() const -> std::weak_ptr<ModelService> {
    return model_service_;
  }

  auto ServiceCenter::getModelServiceObject() const -> QObject* {
    return model_service_.get();
  }

  auto ServiceCenter::getSliceService() const -> std::weak_ptr<SliceService> {
    return slice_service_;
  }

  auto ServiceCenter::getSliceServiceObject() const -> QObject* {
    return slice_service_.get();
  }

  auto ServiceCenter::getProjectService() const -> std::weak_ptr<ProjectService> {
    return project_service_;
  }

  auto ServiceCenter::getProjectServiceObject() const -> QObject* {
    return project_service_.get();
  }

  auto ServiceCenter::getPrinterService() const -> std::weak_ptr<PrinterService> {
    return printer_service_;
  }

  auto ServiceCenter::getPrinterServiceObject() const -> QObject* {
    return printer_service_.get();
  }

  auto ServiceCenter::getModelLibraryService() const -> std::weak_ptr<ModelLibraryService> {
    return model_library_service_;
  }

  auto ServiceCenter::getModelLibraryServiceObject() const -> QObject* {
    return model_library_service_.get();
  }

  auto ServiceCenter::getDownloadService() const -> std::weak_ptr<DownloadService> {
    return download_service_;
  }

  auto ServiceCenter::getDownloadServiceObject() const -> QObject* {
    return download_service_.get();
  }

  auto ServiceCenter::getUpgradeService() const -> std::weak_ptr<UpgradeService> {
    return upgrade_service_;
  }

  auto ServiceCenter::getUpgradeServiceObject() const -> QObject* {
    return upgrade_service_.get();
  }

  auto ServiceCenter::getProfileService() const -> std::weak_ptr<ProfileService> {
    return profile_service_;
  }

  auto ServiceCenter::getProfileServiceObject() const -> QObject* {
    return profile_service_.get();
  }

  bool ServiceCenter::checkNetworkAvailable() const {
    return CheckNetworkAvailable();
  }

  auto ServiceCenter::updateModelRestriction() -> void {
    if (!account_service_->isLogined()) {
      setModelRestriction(DEFAULT_MODEL_RESTRICTION);
      return;
    }

    auto request = http_session_->createRequest<LoadPrivacySettingsRequest>();

    request->setSuccessedCallback([this, request] {
      if (request) {
        setModelRestriction(request->getModelRestriction());
      }
    });

    request->asyncPost();
  }

// ----------- sub servive [end] -----------

}  // namespace cxcloud
