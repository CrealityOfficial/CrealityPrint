#include "cxcloud/service_center.h"

#include <QtCore/QSettings>
#include <QtCore/QLocale>

#include "cxcloud/net/account_request.h"

namespace cxcloud {

ServiceCenter::ServiceCenter(QObject* parent)
    : ServiceCenter({ 0, 0, 0, 0 }, parent) {}

ServiceCenter::ServiceCenter(Version local_version, QObject* parent)
    : QObject(parent)
    , server_list_model_(std::make_unique<ServerListModel>(ServerListModel::DataContainer{
        {
          QString::number(static_cast<int>(ServerType::MAINLAND_CHINA)),
          QStringLiteral("China")
        }, 
         {
          QString::number(static_cast<int>(ServerType::INTERNATIONAL)),
          QStringLiteral("Asia-Pascific")
        },
         {
          QString::number(static_cast<int>(ServerType::INTERNATIONAL)),
          QStringLiteral("Europe")
        },
        {
          QString::number(static_cast<int>(ServerType::INTERNATIONAL)),
          QStringLiteral("North America")
        },
        {
          QString::number(static_cast<int>(ServerType::INTERNATIONAL)),
          QStringLiteral("South America")
        },
         {
          QString::number(static_cast<int>(ServerType::INTERNATIONAL)),
          QStringLiteral("Others")
        },
      }))
    , http_session_(std::make_shared<HttpSession>(local_version, [this]() { return getUrl(); }))
    , account_service_(std::make_shared<AccountService>(http_session_))
    , slice_service_(std::make_shared<SliceService>(http_session_))
    , model_service_(std::make_shared<ModelService>(http_session_))
    , printer_service_(std::make_shared<PrinterService>(http_session_))
    , model_library_service_(std::make_shared<ModelLibraryService>(http_session_))
    , download_service_(std::make_shared<DownloadService>(http_session_))
    , upgrade_service_(std::make_shared<UpgradeService>(http_session_, std::move(local_version))) {
  // register account uid getter and account token getter after account service created.
  http_session_->setAccountUidGetter([this]() { return account_service_->getUserId(); });
  http_session_->setAccountTokenGetter([this]() { return account_service_->getToken(); });
}

void ServiceCenter::initialize() {
  model_service_->setUserIdGetter([this]() { return account_service_->getUserId(); });
  model_library_service_->setModelGroupUrlCreater([this](const QString& group_uid) {
    return QStringLiteral("%1%2").arg(getModelGroupUrlHead()).arg(group_uid);
  });

  connect(account_service_.get(), &AccountService::loginSuccessed,
          download_service_.get(), &DownloadService::fulfillsAllDonwloadPromise);

  connect(model_library_service_.get(), &ModelLibraryService::modelGroupUncollected,
          model_service_.get(), &ModelService::onModelGroupUncollected);
}

void ServiceCenter::uninitialize() {
  account_service_->uninitialize();
  slice_service_->uninitialize();
  model_service_->uninitialize();
  printer_service_->uninitialize();
  model_library_service_->uninitialize();
  download_service_->uninitialize();
  upgrade_service_->uninitialize();
}

void ServiceCenter::loadUserSettings() {
  account_service_->initialize();
  slice_service_->initialize();
  model_service_->initialize();
  printer_service_->initialize();
  model_library_service_->initialize();
  download_service_->initialize();
  upgrade_service_->initialize();
}

void ServiceCenter::setOpenFileHandler(std::function<void(const QString&)> handler) {
  open_file_handler_ = handler;
  slice_service_->setOpenFileHandler(handler);
  download_service_->setOpenFileHandler(handler);
}

void ServiceCenter::setOpenFileListHandler(std::function<void(const QStringList&)> handler) {
  open_file_list_handler_ = handler;
  download_service_->setOpenFileListHandler(handler);
}

void ServiceCenter::setSelectedModelCombineSaver(std::function<QString(const QString&)> saver) {
  selected_model_combine_saver_ = saver;
  model_service_->setSelectedModelCombineSaver(saver);
}

void ServiceCenter::setSelectedModelUncombineSaver(std::function<QStringList(const QString&)> saver) {
  selected_model_uncombine_saver_ = saver;
  model_service_->setSelectedModelUncombineSaver(saver);
}

void ServiceCenter::setCurrentSliceSaver(std::function<QString(const QString&)> saver) {
  current_slice_saver_ = saver;
  slice_service_->setCurrentSliceSaver(saver);
}

QStringList ServiceCenter::getVaildSliceFileSuffixList() const {
  return vaild_slice_file_suffix_list_;
}

void ServiceCenter::setVaildSliceFileSuffixList(const QStringList& list) {
  vaild_slice_file_suffix_list_ = list;
  slice_service_->setVaildSuffixList(list);
}

bool ServiceCenter::isOnlineSliceFileCompressed() const {
  return online_slice_file_compressed_;
}

void ServiceCenter::setOnlineSliceFileCompressed(bool compressed) {
  online_slice_file_compressed_ = compressed;
  slice_service_->setOnlineSliceFileCompressed(compressed);
}

QString ServiceCenter::getModelCacheDirPath() const {
  return model_cache_dir_path_;
}

void ServiceCenter::setModelCacheDirPath(const QString& path) {
  model_cache_dir_path_ = path;
  model_library_service_->setCacheDirPath(path);
  download_service_->setCacheDirPath(path);
}

size_t ServiceCenter::getMaxDownloadThreadCount() const {
  return max_download_thread_count_;
}

void ServiceCenter::setMaxDownloadThreadCount(size_t count) {
  max_download_thread_count_ = count;
  download_service_->setMaxDownloadThreadCount(count);
}

// ----------- base [begin] -----------

ApplicationType ServiceCenter::getApplicationType() const {
  return http_session_->getApplicationType();
}

void ServiceCenter::setApplicationType(ApplicationType type) {
  http_session_->setApplicationType(type);
}

ServerType ServiceCenter::getServerType() const {
  return static_cast<ServerType>(getServerTypeIndex());
}

void ServiceCenter::setServerType(ServerType server_type) {
  setServerTypeIndex(static_cast<int>(server_type));
}

int ServiceCenter::getServerTypeIndex() const {
  const auto default_type = QLocale{}.country() == QLocale::Country::China
                               ? static_cast<int>(ServerType::MAINLAND_CHINA)
                               : static_cast<int>(ServerType::INTERNATIONAL);
  QSettings setting;
  setting.beginGroup(QStringLiteral("cloud_service"));
  auto server_type = setting.value(QStringLiteral("server_type"), default_type).toInt();
  setting.endGroup();
  return server_type;
}

void ServiceCenter::setServerTypeIndex(int server_type) {
  QSettings setting;
  setting.beginGroup(QStringLiteral("cloud_service"));
  setting.setValue(QStringLiteral("server_type"), server_type);
  setting.endGroup();
  Q_EMIT serverTypeChanged();
}

QAbstractListModel* ServiceCenter::getServerListModel() {
  return server_list_model_.get();
}

QString ServiceCenter::getUrl() const {
  switch (getServerType()) {
    case ServerType::MAINLAND_CHINA:
      return QStringLiteral("https://www.crealitycloud.cn");
      break;
    case ServerType::INTERNATIONAL:
    default:
      return QStringLiteral("https://www.crealitycloud.com");
      break;
  }
}

QString ServiceCenter::getModelGroupUrlHead() const {
  switch (getServerType()) {
    case ServerType::MAINLAND_CHINA:
      return QStringLiteral("https://www.crealitycloud.cn/model-detail/");
      break;
    case ServerType::INTERNATIONAL:
    default:
      return QStringLiteral("https://www.crealitycloud.com/model-detail/");
      break;
  }
}

QString ServiceCenter::loadModelGroupUrl(const QString& group_uid) {
  return QStringLiteral("%1%2").arg(getModelGroupUrlHead()).arg(group_uid);
}

// ----------- base [end] -----------

// ----------- sub servive [begin] -----------

std::weak_ptr<AccountService> ServiceCenter::getAccountService() const {
  return account_service_;
}

QObject* ServiceCenter::getAccountServiceObject() const {
  return account_service_.get();
}

std::weak_ptr<ModelService> ServiceCenter::getModelService() const {
  return model_service_;
}

QObject* ServiceCenter::getModelServiceObject() const {
  return model_service_.get();
}

std::weak_ptr<SliceService> ServiceCenter::getSliceService() const {
  return slice_service_;
}

QObject* ServiceCenter::getSliceServiceObject() const {
  return slice_service_.get();
}

std::weak_ptr<PrinterService> ServiceCenter::getPrinterService() const {
  return printer_service_;
}

QObject* ServiceCenter::getPrinterServiceObject() const {
  return printer_service_.get();
}

std::weak_ptr<ModelLibraryService> ServiceCenter::getModelLibraryService() const {
  return model_library_service_;
}

QObject* ServiceCenter::getModelLibraryServiceObject() const {
  return model_library_service_.get();
}

std::weak_ptr<DownloadService> ServiceCenter::getDownloadService() const {
  return download_service_;
}

QObject* ServiceCenter::getDownloadServiceObject() const {
  return download_service_.get();
}

std::weak_ptr<UpgradeService> ServiceCenter::getUpgradeService() const {
  return upgrade_service_;
}

QObject* ServiceCenter::getUpgradeServiceObject() const {
  return upgrade_service_.get();
}

// ----------- sub servive [end] -----------

}  // namespace cxcloud
