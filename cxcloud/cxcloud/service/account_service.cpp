#include "cxcloud/service/account_service.h"
#include "cxcloud/service_center.h"
#include <QtCore/QSettings>
#include <QtCore/QUrl>
#include <QtGui/QDesktopServices>
namespace cxcloud {

AccountService::AccountService(std::weak_ptr<HttpSession> http_session, QObject* parent)
    : BaseService(http_session, parent)
    , qrcode_image_provider_(new QrcodeImageProvider)  // qml engine will takes the ownership
    , qrcode_login_request_(std::make_unique<QrcodeLoginLoopRequest>(
        [this](const QString& url) {
          auto const provider = getQrcodeImageProvider();
          if (provider) { provider->setUrl(url); }
          Q_EMIT qrcodeRefreshed();
        },
        [this](const UserBaseInfo& base_info) {
          loadUserDetailInfo(base_info);
        })) {}

void AccountService::initialize() {
  QSettings setting;
  setting.beginGroup(QStringLiteral("cloud_service"));
  QString token = setting.value(QStringLiteral("token"), {}).toString();
  QString user_id = setting.value(QStringLiteral("user_id"), {}).toString();
  bool auto_login = setting.value(QStringLiteral("auto_login"), true).toBool();
  setting.endGroup();

  if (!token.isEmpty() && !user_id.isEmpty()) {
    UserBaseInfo info;
    info.token = token;
    info.user_id = user_id;
    if (auto_login) {
      loadUserDetailInfo(info);
    }
  }
}

void AccountService::uninitialize() {
  stopAutoRefreshQrcode();
}

QPointer<QrcodeImageProvider> AccountService::getQrcodeImageProvider() const {
  return qrcode_image_provider_;
}

bool AccountService::isLogined() const {
  return !base_info_.token.isEmpty() &&
         !base_info_.user_id.isEmpty();
}

const UserBaseInfo& AccountService::userBaseInfo() const {
  return base_info_;
}

const UserDetailInfo& AccountService::userDetailInfo() const {
  return detail_info_;
}

QString AccountService::getToken() const {
  return base_info_.token;
}

QString AccountService::getUserId() const {
  return base_info_.user_id;
}

QString AccountService::getNickName() const {
  return detail_info_.nick_name;
}

QString AccountService::getAvatar() const {
  return detail_info_.avatar;
}

double AccountService::getUsedStorageSpace() const {
  return detail_info_.used_storage_space;
}

double AccountService::getMaxStorageSpace() const {
  return detail_info_.max_storage_space;
}

void AccountService::requestVerifyCode(const QString& phone_number, const QString& area_code) {
  auto* request = createRequest<LoadVerificationCodeRequest>(phone_number, area_code);
  request->setSuccessedCallback([this]() { Q_EMIT requestVerifyCodeSuccessed(); });
  request->setFailedCallback([this]() { Q_EMIT requestVerifyCodeFailed(); });
  HttpPost(request);
}

void AccountService::startAutoRefreshQrcode() {
  getHttpSession().lock()->addRequest(qrcode_login_request_.get());
  qrcode_login_request_->startWorking();
}

void AccountService::stopAutoRefreshQrcode() {
  qrcode_login_request_->stopWorking();
}

void AccountService::loginByMobilePassword(const QString& phone_number,
                                           const QString& area_code,
                                           const QString& password,
                                           bool auto_login) {
  startAccountLoginRequest(
    createRequest<PhoneLoginRequest>(phone_number, area_code, password), auto_login);
}

void AccountService::loginByEmailPassword(const QString& email,
                                          const QString& password,
                                          bool auto_login) {
  startAccountLoginRequest(createRequest<EmailLoginRequest>(email, password), auto_login);
}

void AccountService::loginByMobileVerifyCode(const QString& phone_number,
                                             const QString& area_code,
                                             const QString& verify_code,
                                             bool auto_login) {
  auto* request = createRequest<VerificationCodeLoginRequest>(phone_number, area_code, verify_code);

  request->setSuccessedCallback([this, request, auto_login]() {
    QSettings setting;
    setting.beginGroup(QStringLiteral("cloud_service"));
    setting.setValue(QStringLiteral("auto_login"), auto_login);
    setting.endGroup();

    loadUserDetailInfo(request->getBaseInfo());
  });

  request->setFailedCallback(
    [this, request]() { Q_EMIT loginFailed(QVariant::fromValue(request->getResponseMessage())); });

  HttpPost(request);
}

void AccountService::logout() {
  bool need_emit = isLogined();

  base_info_ = UserBaseInfo{};
  detail_info_ = UserDetailInfo{};

  QSettings setting;
  setting.beginGroup(QStringLiteral("cloud_service"));
  setting.setValue(QStringLiteral("auto_login"), false);
  setting.endGroup();

  if (need_emit) {
    emit loginedChanged();
  }
}

void AccountService::startAccountLoginRequest(QPointer<AccountLoginRequest> request,
                                              bool auto_login) {
  if (!request) { return; }

  request->setSuccessedCallback([this, request, auto_login]() {
    QSettings setting;
    setting.beginGroup(QStringLiteral("cloud_service"));
    setting.setValue(QStringLiteral("auto_login"), auto_login);
    setting.endGroup();

    loadUserDetailInfo(request->getBaseInfo());
  });

  request->setFailedCallback(
    [this, request]() { Q_EMIT loginFailed(QVariant::fromValue(request->getResponseMessage())); });

  request->setErredCallback([this, request]() {
    if (request->getResponseState() == cpr::ErrorCode::HOST_RESOLUTION_FAILURE) {
      Q_EMIT loginFailed(QVariant::fromValue(QStringLiteral("HOST_RESOLUTION_FAILURE")));
    }
  });

  HttpPost(request.data());
}

void AccountService::loadUserDetailInfo(const UserBaseInfo& base_info) {
  auto* request = createRequest<LoadUserDetailInfoRequest>(base_info);

  request->setSuccessedCallback([this, request, base_info]() {
    base_info_ = base_info;
    detail_info_ = request->getDetailInfo();

    QSettings setting;
    setting.beginGroup(QStringLiteral("cloud_service"));
    setting.setValue(QStringLiteral("token"), base_info_.token);
    setting.setValue(QStringLiteral("user_id"), base_info_.user_id);
    setting.endGroup();

    Q_EMIT loginedChanged();
    Q_EMIT loginSuccessed();

    // download_service_->fulfillsAllDonwloadPromise();
  });

  HttpPost(request);
}

void AccountService::resetPassWordWebsitebyPhone() 
{
    QSettings setting;
    setting.beginGroup(QStringLiteral("cloud_service"));
    auto server_type = setting.value(QStringLiteral("server_type"), 0).toInt();

    setting.endGroup();
    QString suffix = "?resetpassword=1&channel=creality-print";
    QString url;
    if (server_type !=0) {
        url = "https://www.crealitycloud.com" +suffix;
    }
    else {
        url = "https://www.crealitycloud.cn" + suffix;;
    }
    QDesktopServices::openUrl(QUrl(url));
}

void AccountService::resetPassWordWebsitebyMail()
{
    QSettings setting;
    setting.beginGroup(QStringLiteral("cloud_service"));
    auto server_type = setting.value(QStringLiteral("server_type"), 0).toInt();
    setting.endGroup();
    QString suffix = "?resetpassword=0&channel=creality-print";
    QString url;
    if (server_type != 0) {
        url = "https://www.crealitycloud.com" + suffix;
    }
    else {
        url = "https://www.crealitycloud.cn" + suffix;;
    }
    QDesktopServices::openUrl(QUrl(url));
}

void AccountService::signupWebsitebyPhone()
{
    QSettings setting;
    setting.beginGroup(QStringLiteral("cloud_service"));
    auto server_type = setting.value(QStringLiteral("server_type"), 0).toInt();
    setting.endGroup();
    QString suffix = "?signup=1&channel=creality-print";
    QString url;
    if (server_type != 0) {
        url = "https://www.crealitycloud.com" + suffix;
    }
    else {
        url = "https://www.crealitycloud.cn" + suffix;;
    }
    QDesktopServices::openUrl(QUrl(url));
}

void AccountService::signupWebsitebyMail()
{
    QSettings setting;
    setting.beginGroup(QStringLiteral("cloud_service"));
    auto server_type = setting.value(QStringLiteral("server_type"), 0).toInt();
    setting.endGroup();
    QString suffix = "?signup=0&channel=creality-print";
    QString url;
    if (server_type != 0) {
        url = "https://www.crealitycloud.com" + suffix;
    }
    else {
        url = "https://www.crealitycloud.cn" + suffix;;
    }
    QDesktopServices::openUrl(QUrl(url));
}


}  // namespace cxcloud
