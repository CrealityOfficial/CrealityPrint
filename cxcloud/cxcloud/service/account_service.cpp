#include "cxcloud/service/account_service.h"

#include <QtGui/QDesktopServices>

#include "cxcloud/define.h"
#include "cxcloud/tool/settings.h"

namespace cxcloud {

  AccountService::AccountService(std::weak_ptr<HttpSession> http_session, QObject* parent)
      : BaseService{ http_session, parent }
      , qrcode_image_provider_{ new QrcodeImageProvider }  // qml engine will takes the ownership
      , qrcode_login_request_{ std::make_unique<QrcodeLoginLoopRequest>(
          [this](const QString& url) {
            const auto provider = getQrcodeImageProvider();
            if (provider) { provider->setUrl(url); }
            qrcodeRefreshed();
          },
          [this](const UserBaseInfo& base_info) {
            loadUserDetailInfo(base_info);
          }) } {}

  auto AccountService::initialize() -> void {
    auto settings = CloudSettings{};
    auto token = settings.value(QStringLiteral("token"), {}).toString();
    auto user_id = settings.value(QStringLiteral("user_id"), {}).toString();
    auto auto_login = settings.value(QStringLiteral("auto_login"), true).toBool();

    if (!token.isEmpty() && !user_id.isEmpty()) {
      auto info = UserBaseInfo{};
      info.token = token;
      info.user_id = user_id;
      if (auto_login) {
        loadUserDetailInfo(info);
      }
    }
  }

  auto AccountService::uninitialize() -> void {
    stopAutoRefreshQrcode();
  }

  auto AccountService::getQrcodeImageProvider() const -> QPointer<QrcodeImageProvider> {
    return qrcode_image_provider_;
  }

  auto AccountService::isLogined() const -> bool {
    return !base_info_.token.isEmpty() && !base_info_.user_id.isEmpty();
  }

  auto AccountService::userBaseInfo() const -> const UserBaseInfo& {
    return base_info_;
  }

  auto AccountService::userDetailInfo() const -> const UserDetailInfo& {
    return detail_info_;
  }

  auto AccountService::getToken() const -> QString {
    return base_info_.token;
  }

  auto AccountService::getUserId() const -> QString {
    return base_info_.user_id;
  }

  auto AccountService::getNickName() const -> QString {
    return detail_info_.nick_name;
  }

  auto AccountService::getAvatar() const -> QString {
    return detail_info_.avatar;
  }

  auto AccountService::getUsedStorageSpace() const -> double {
    return detail_info_.used_storage_space;
  }

  auto AccountService::getMaxStorageSpace() const -> double {
    return detail_info_.max_storage_space;
  }

  auto AccountService::requestVerifyCode(const QString& phone_number,
                                         const QString& area_code) -> void {
    auto request = createRequest<LoadVerificationCodeRequest>(phone_number, area_code);
    request->setSuccessedCallback([this]() { requestVerifyCodeSuccessed(); });
    request->setFailedCallback([this]() { requestVerifyCodeFailed(); });
    request->asyncPost();
  }

  auto AccountService::startAutoRefreshQrcode() -> void {
    getHttpSession().lock()->addRequest(qrcode_login_request_.get());
    qrcode_login_request_->startWorking();
  }

  auto AccountService::stopAutoRefreshQrcode() -> void {
    qrcode_login_request_->stopWorking();
  }

  auto AccountService::loginByMobilePassword(const QString& phone_number,
                                             const QString& area_code,
                                             const QString& password,
                                             bool auto_login) -> void {
    startAccountLoginRequest(
      createRequest<PhoneLoginRequest>(phone_number, area_code, password).data(), auto_login);
  }

  auto AccountService::loginByEmailPassword(const QString& email,
                                            const QString& password,
                                            bool auto_login) -> void {
    startAccountLoginRequest(createRequest<EmailLoginRequest>(email, password).data(), auto_login);
  }

  auto AccountService::loginByMobileVerifyCode(const QString& phone_number,
                                               const QString& area_code,
                                               const QString& verify_code,
                                               bool auto_login) -> void {
    auto request = createRequest<VerificationCodeLoginRequest>(phone_number,
                                                               area_code,
                                                               verify_code);

    request->setSuccessedCallback([this, request, auto_login]() {
      CloudSettings{}.setValue(QStringLiteral("auto_login"), auto_login);
      loadUserDetailInfo(request->getBaseInfo());
    });

    request->setFailedCallback([this, request]() {
      loginFailed(QVariant::fromValue(request->getResponseMessage()));
    });

    request->asyncPost();
  }

  auto AccountService::logout() -> void {
    const auto need_emit = isLogined();

    base_info_ = UserBaseInfo{};
    detail_info_ = UserDetailInfo{};

    CloudSettings{}.setValue(QStringLiteral("auto_login"), false);

    if (need_emit) {
      loginedChanged();
      logouted();
    }
  }

  auto AccountService::startAccountLoginRequest(QPointer<AccountLoginRequest> request,
                                                bool auto_login) -> void {
    if (!request) {
      return;
    }

    request->setSuccessedCallback([this, request, auto_login]() {
      CloudSettings{}.setValue(QStringLiteral("auto_login"), auto_login);
      loadUserDetailInfo(request->getBaseInfo());
    });

    request->setFailedCallback([this, request]() {
      loginFailed(QVariant::fromValue(request->getResponseMessage()));
    });

    request->setErredCallback([this, request]() {
      if (request->getResponseState() == HttpErrorCode::HOST_RESOLUTION_FAILURE) {
        loginFailed(QVariant::fromValue(QStringLiteral("HOST_RESOLUTION_FAILURE")));
      }
    });

    request->asyncPost();
  }

  auto AccountService::loadUserDetailInfo(const UserBaseInfo& base_info) -> void {
    auto request = createRequest<LoadUserDetailInfoRequest>(base_info);

    request->setSuccessedCallback([this, request, base_info]() {
      base_info_ = base_info;
      detail_info_ = request->getDetailInfo();

      auto settings = CloudSettings{};
      settings.setValue(QStringLiteral("token"), base_info_.token);
      settings.setValue(QStringLiteral("user_id"), base_info_.user_id);

      loginedChanged();
      loginSuccessed();
    });

    request->asyncPost();
  }

  void AccountService::resetPassWordWebsitebyPhone() {
    auto server_type = static_cast<int>(CloudSettings{}.getServerType());

    static const QString suffix = "?resetpassword=1&channel=creality-print";

    QString url;
    if (server_type != 0) {
      url = "https://www.crealitycloud.com" + suffix;
    } else {
      url = "https://www.crealitycloud.cn" + suffix;
    }

    QDesktopServices::openUrl(QUrl{ url });
  }

  void AccountService::resetPassWordWebsitebyMail() {
    auto server_type = static_cast<int>(CloudSettings{}.getServerType());

    static const QString suffix = "?resetpassword=0&channel=creality-print";

    QString url;
    if (server_type != 0) {
      url = "https://www.crealitycloud.com" + suffix;
    } else {
      url = "https://www.crealitycloud.cn" + suffix;
    }

    QDesktopServices::openUrl(QUrl{ url });
  }

  void AccountService::signupWebsitebyPhone() {
    auto server_type = static_cast<int>(CloudSettings{}.getServerType());

    static const QString suffix = "?signup=1&channel=creality-print";

    QString url;
    if (server_type != 0) {
      url = "https://www.crealitycloud.com" + suffix;
    } else {
      url = "https://www.crealitycloud.cn" + suffix;
    }

    QDesktopServices::openUrl(QUrl{ url });
  }

  void AccountService::signupWebsitebyMail() {
    auto server_type = static_cast<int>(CloudSettings{}.getServerType());

    static const QString suffix = "?signup=0&channel=creality-print";

    QString url;
    if (server_type != 0) {
      url = "https://www.crealitycloud.com" + suffix;
    } else {
      url = "https://www.crealitycloud.cn" + suffix;
    }

    QDesktopServices::openUrl(QUrl{ url });
  }

}  // namespace cxcloud
