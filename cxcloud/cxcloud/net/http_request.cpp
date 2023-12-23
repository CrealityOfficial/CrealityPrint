#include "cxcloud/net/http_request.h"

#include <QtCore/QByteArray>
#include <QtCore/QCoreApplication>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonDocument>
#include <QtCore/QSettings>
#include <QtCore/QUuid>
#include <QtNetwork/QNetworkInterface>

#include <boost/asio.hpp>
#include <boost/asio/thread_pool.hpp>

#include <qtusercore/module/systemutil.h>

namespace cxcloud {

namespace _intrenel {

inline std::string GetDuid() {
  for (const auto& network_interface : QNetworkInterface::allInterfaces()) {
    const auto flags = network_interface.flags();
    if (flags.testFlag(QNetworkInterface::InterfaceFlag::IsUp) &&
        flags.testFlag(QNetworkInterface::InterfaceFlag::IsRunning) &&
        !flags.testFlag(QNetworkInterface::InterfaceFlag::IsLoopBack)) {
      return network_interface.hardwareAddress().toStdString();
    }
  }

  return {};
}

inline std::string GetUuid() {
  return QUuid::createUuid().toString().toStdString();
}

inline std::string I18nToLangaugeIndex(const std::string& i18n) {
  static const std::map<std::string, std::string> I18N_INDEX_MAP{
    { "en"   , "0" },
    { "zh_cn", "1" },
    { "zh_tw", "2" },
  };

  auto iter = I18N_INDEX_MAP.find(i18n);
  if (iter == I18N_INDEX_MAP.cend()) {
    return "0";
  }

  return iter->second;
}

inline std::string GetLanguageIndex() {
  QSettings setting;
  setting.beginGroup(QStringLiteral("language_perfer_config"));
  std::string i18n = setting.value(QStringLiteral("language_type"))
                            .toString()
                            .toLower()
                            .remove(QStringLiteral(".ts"))
                            .toStdString();
  setting.endGroup();
  return I18nToLangaugeIndex(i18n);
}

}  // namespace _intrenel

void HttpGet(QPointer<HttpRequest> request) {
  if (!request || !request->isInitlized()) { return; }

  request->future_ = std::async(std::launch::async, [request]() {
    cpr::Session session;
    session.SetUrl(request->getUrl().toStdString());
    session.UpdateHeader(request->getHeader());
    session.SetParameters(request->getParameters());

    request->response_ = session.Get();
    request->setResponseState(request->response_.error.code);
    request->setResponseMessage(QString::fromStdString(request->response_.error.message));
    request->setResponseData(request->response_.text.c_str());
    request->callWhenResponsed();
    request->callAfterResponsed();
  });
}

void HttpSyncPost(QPointer<HttpRequest> request) {
  if (!request || !request->isInitlized()) { return; }

  cpr::Session session;
  session.SetUrl(request->getUrl().toStdString());
  session.UpdateHeader(request->getHeader());
  session.SetBody(request->getRequestData().toStdString());

  request->response_ = session.Post();
  request->setResponseState(request->response_.error.code);
  request->setResponseMessage(QString::fromStdString(request->response_.error.message));
  request->setResponseData(request->response_.text.c_str());
  request->callWhenResponsed();
  request->callAfterResponsed();
}

void HttpPost(QPointer<HttpRequest> request) {
  if (!request || !request->isInitlized()) { return; }

  request->future_ = std::async(std::launch::async, [request]() {
    HttpSyncPost(request);
  });
}

void HttpSyncDownload(QPointer<HttpRequest> request) {
  if (!request || !request->isInitlized()) { return; }

  QString download_url = request->getDownloadSrcUrl();
  QString download_path = request->getDownloadDstPath();
  if (download_url.isEmpty() || download_path.isEmpty()) { return; }

  mkMutiDir(QFileInfo{ download_path }.absolutePath());

  qDebug().noquote() << QStringLiteral("---------- cxcloud::HttpDownload ----------\n")
                     << QStringLiteral("url: %1\n").arg(download_url)
                     << QStringLiteral("file: %1").arg(download_path);

  cpr::Session session;
  session.SetVerifySsl(false);
  session.SetUrl(download_url.toStdString());
  session.SetProgressCallback({ [request](cpr::cpr_off_t download_total,
                                          cpr::cpr_off_t download_current,
                                          cpr::cpr_off_t upload_total,
                                          cpr::cpr_off_t upload_current,
                                          intptr_t userdata) -> bool {
    // @see https://docs.libcpr.org/advanced-usage.html#download-file
    // Return `true` on success, or `false` to **cancel** the transfer.
    if (!request || request->isCancelDownloadLater()) {
      return false;
    }

    request->setTotalProgress(download_total);
    request->setCurrentProgress(download_current);
    request->callWhenProgressUpdated();
    request->callAfterProgressUpdated();
    return true;
  } });

  request->response_ = session.Download(download_path.toStdString());
  request->setResponseData(QJsonDocument{ QJsonObject{
    { QStringLiteral("code"), QString::number(request->response_.error ? 1 : 0) }
  } }.toJson(QJsonDocument::JsonFormat::Compact));
  request->callWhenResponsed();
  request->callAfterResponsed();
}

void HttpDownload(QPointer<HttpRequest> request) {
  if (!request || !request->isInitlized()) { return; }

  request->future_ = std::async(std::launch::async, [request]() {
    HttpSyncDownload(request);
  });
}

void HttpDownload(QPointer<HttpRequest> request, boost::asio::thread_pool& thread_pool) {
  if (!request || !request->isInitlized()) { return; }

  request->future_ = boost::asio::post(thread_pool, std::packaged_task<void()>{ [request]() {
    HttpSyncDownload(request);
  } });
}





HttpSession::HttpSession(Version application_version,
                         std::function<QString(void)> url_head_getter,
                         std::function<QString(void)> account_uid_getter,
                         std::function<QString(void)> account_token_getter,
                         QObject* parent)
    : QObject(parent)
    , application_version_(std::move(application_version))
    , url_head_getter_(url_head_getter)
    , account_uid_getter_(account_uid_getter)
    , account_token_getter_(account_token_getter) {}

Version HttpSession::getApplicationVersion() const {
  return application_version_;
}

void HttpSession::setApplicationVersion(const Version& version) {
  application_version_ = version;
}

ApplicationType HttpSession::getApplicationType() const {
  return application_type_;
}

void HttpSession::setApplicationType(ApplicationType type) {
  application_type_ = type;
}

std::function<QString(void)> HttpSession::getUrlHeadGetter() const {
  return url_head_getter_;
}

void HttpSession::setUrlHeadGetter(std::function<QString(void)> url_head_getter) {
  url_head_getter_ = url_head_getter;
}

std::function<QString(void)> HttpSession::getAccountUidGetter() const {
  return account_uid_getter_;
}

void HttpSession::setAccountUidGetter(std::function<QString(void)> account_uid_getter) {
  account_uid_getter_ = account_uid_getter;
}

std::function<QString(void)> HttpSession::getAccountTokenGetter() const {
  return account_token_getter_;
}

void HttpSession::setAccountTokenGetter(std::function<QString(void)> account_token_getter) {
  account_token_getter_ = account_token_getter;
}





bool HttpRequest::isInitlized() {
  return !getUrl().isEmpty();
}

QByteArray HttpRequest::getRequestData() const {
  return QByteArrayLiteral("{}");
}

QString HttpRequest::getDownloadSrcUrl() const {
  return QStringLiteral("");
}

QString HttpRequest::getDownloadDstPath() const {
  return QStringLiteral("");
}

Version HttpRequest::getApplicationVersion() const {
  return application_version_;
}

ApplicationType HttpRequest::getApplicationType() const {
  return application_type_;
}

QString HttpRequest::getAccountUid() const {
  return account_uid_;
}

QString HttpRequest::getAccountToken() const {
  return account_token_;
}

cpr::Header HttpRequest::getHeader() const {
  if (!header_.empty()) {
    return header_;
  }

  header_ = {
    { "Content-Type"    , "application/json" },
    { "__CXY_APP_ID_"   , "creality_model" },
    { "__CXY_APP_VER_"  , getApplicationVersion().toString().toStdString() },
    { "__CXY_DUID_"     , _intrenel::GetDuid() },
#if defined(Q_OS_WINDOWS)
    { "__CXY_OS_VER_"   , "win" },
#elif defined(Q_OS_LINUX)
    { "__CXY_OS_VER_"   , "linux" },
#elif defined(Q_OS_MACOS)
    { "__CXY_OS_VER_"   , "macos" },
#endif
    { "__CXY_OS_LANG_"  , _intrenel::GetLanguageIndex() },
    { "__CXY_PLATFORM_" , [this]() -> std::string {
      switch (getApplicationType()) {
        case ApplicationType::CREATIVE_PRINT:
          return "11";
        case ApplicationType::HALOT_BOX:
        default:
          return "12";
      }
    }() },
    { "__CXY_REQUESTID_", _intrenel::GetUuid() },
  };

  using pair_t = cpr::Header::value_type;

  auto token = getAccountToken().toStdString();
  if (!token.empty()) {
    header_.emplace(pair_t{"__CXY_TOKEN_", std::move(token)});
  }

  auto uid = getAccountUid().toStdString();
  if (!uid.empty()) {
    header_.emplace(pair_t{"__CXY_UID_", std::move(uid)});
  }

  return header_;
}

void HttpRequest::setParameters(cpr::Parameters parameters) {
  parameters_ = parameters;
}

cpr::Parameters HttpRequest::getParameters() const {
  return parameters_;
}

QString HttpRequest::getUrlHead() const {
  return url_head_;
}

QString HttpRequest::getUrlTail() const {
  return {};
}

QString HttpRequest::getUrl() const {
  return getUrlHead() + getUrlTail();
}

void HttpRequest::callWhenSuccessed() {}

void HttpRequest::callWhenFailed() {}

void HttpRequest::callWhenErred() {}

void HttpRequest::callWhenProgressUpdated() {}

bool HttpRequest::isAutoDelete() const {
  return true;
}

bool HttpRequest::handleResponseData() {
  bool successed{ true };
  QByteArray bytes = getResponseData();
  QJsonObject object{};
  QString message{};

  QJsonParseError error;
  const QJsonDocument document = QJsonDocument::fromJson(bytes, &error);
  successed = error.error == QJsonParseError::ParseError::NoError;

  if (!successed) {
    message = error.errorString();

  } else {
    object = document.object();
    successed = object.value(QStringLiteral("code")).toInt() == 0;

    if (!successed) {
      message = object.value(QStringLiteral("msg")).toString();
    }
  }

  setResponseJson(object);
  setResponseMessage(message);

  return successed;
}

void HttpRequest::callWhenResponsed() {
  const auto handle_with_state = [this](bool successed) {
    QMetaObject::invokeMethod(this, [this, successed]() {
      successed ? callWhenSuccessed() : callWhenFailed();
      successed ? callAfterSuccessed() : callAfterFailed();
      if (isAutoDelete()) { deleteLater(); }
    }, Qt::ConnectionType::QueuedConnection);
  };

  if (response_.error.code != cpr::ErrorCode::OK) {
    handle_with_state(false);
    return;
  }

  if (!handleResponseData()) {
    handle_with_state(false);
    return;
  }

  handle_with_state(true);
}

size_t HttpRequest::getTotalProgress() const {
  return total_progress_;
}

size_t HttpRequest::getCurrentProgress() const {
  return current_progress_;
}

bool HttpRequest::isCancelDownloadLater() const {
  return cancel_download_later_.load();
}

void HttpRequest::setCancelDownloadLater(bool cancel) {
  cancel_download_later_.store(cancel);
}

const QByteArray& HttpRequest::getResponseData() const {
  return response_data_;
}

const QJsonObject& HttpRequest::getResponseJson() const {
  return response_json_;
}

cpr::ErrorCode HttpRequest::getResponseState() const {
  return response_state_;
}

const QString& HttpRequest::getResponseMessage() const {
  return response_message_;
}

void HttpRequest::setTotalProgress(size_t progress) {
  total_progress_ = progress;
}

void HttpRequest::setCurrentProgress(size_t progress) {
  current_progress_ = progress;
}

void HttpRequest::setResponseData(const QByteArray& data) {
  response_data_ = data;
}

void HttpRequest::setResponseJson(const QJsonObject& json) {
  response_json_ = json;
}

void HttpRequest::setResponseState(const cpr::ErrorCode& state) {
  response_state_ = state;
}

void HttpRequest::setResponseMessage(const QString& message) {
  response_message_ = message;
}

void HttpRequest::setResponsedCallback(std::function<void(void)> callback) {
  after_responsed_callback_ = callback;
}

void HttpRequest::setSuccessedCallback(std::function<void(void)> callback) {
  after_successed_callback_ = callback;
}

void HttpRequest::setFailedCallback(std::function<void(void)> callback) {
  after_failed_callback_ = callback;
}

void HttpRequest::setErredCallback(std::function<void(void)> callback) {
  after_erred_callback_ = callback;
}

void HttpRequest::setProgressUpdatedCallback(std::function<void(void)> callback) {
  after_progress_updated_callback_ = callback;
}

void HttpRequest::callAfterResponsed() {
  if (after_responsed_callback_) { after_responsed_callback_(); }
}

void HttpRequest::callAfterSuccessed() {
  if (after_successed_callback_) { after_successed_callback_(); }
}

void HttpRequest::callAfterFailed() {
  if (after_failed_callback_) { after_failed_callback_(); }
}

void HttpRequest::callAfterErred() {
  if (after_erred_callback_) { after_erred_callback_(); }
}

void HttpRequest::callAfterProgressUpdated() {
  if (after_progress_updated_callback_) { after_progress_updated_callback_(); }
}

}  // namespace cxcloud
