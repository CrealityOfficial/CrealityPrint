#pragma once

#ifndef CXCLOUD_SERVICE_ACCOUNT_SERVICE_H_
#define CXCLOUD_SERVICE_ACCOUNT_SERVICE_H_

#include <memory>

#include <QtCore/QObject>
#include <QtCore/QPointer>

#include "cxcloud/define.h"
#include "cxcloud/interface.hpp"
#include "cxcloud/net/account_request.h"
#include "cxcloud/tool/qrcode_image_provider.h"
#include "cxcloud/service/base_service.h"

namespace cxcloud {

  class CXCLOUD_API AccountService : public BaseService {
    Q_OBJECT;

   public:
    explicit AccountService(std::weak_ptr<HttpSession> http_session, QObject* parent = nullptr);
    virtual ~AccountService() = default;

   public:
    auto initialize() -> void override;
    auto uninitialize() -> void override;

    auto getQrcodeImageProvider() const -> QPointer<QrcodeImageProvider>;

   public:
    auto isLogined() const -> bool;
    Q_SIGNAL void loginedChanged();
    Q_PROPERTY(bool logined READ isLogined NOTIFY loginedChanged);

    auto userBaseInfo() const -> const UserBaseInfo&;
    auto userDetailInfo() const -> const UserDetailInfo&;

    auto getToken() const -> QString;
    Q_PROPERTY(QString token READ getToken NOTIFY loginedChanged);

    auto getUserId() const -> QString;
    Q_PROPERTY(QString userId READ getUserId NOTIFY loginedChanged);

    auto getNickName() const -> QString;
    Q_PROPERTY(QString nickName READ getNickName NOTIFY loginedChanged);

    auto getAvatar() const -> QString;
    Q_PROPERTY(QString avatar READ getAvatar NOTIFY loginedChanged);

    auto getUsedStorageSpace() const -> double;
    Q_PROPERTY(double usedStorageSpace READ getUsedStorageSpace NOTIFY loginedChanged);

    auto getMaxStorageSpace() const -> double;
    Q_PROPERTY(double maxStorageSpace READ getMaxStorageSpace NOTIFY loginedChanged);

    Q_INVOKABLE void requestVerifyCode(const QString& phone_number, const QString& area_code);
    Q_SIGNAL void requestVerifyCodeSuccessed();
    Q_SIGNAL void requestVerifyCodeFailed();

    Q_INVOKABLE void startAutoRefreshQrcode();
    Q_INVOKABLE void stopAutoRefreshQrcode();
    Q_SIGNAL void qrcodeRefreshed();

    Q_INVOKABLE void loginByMobilePassword(const QString& phone_number,
                                           const QString& area_code,
                                           const QString& password,
                                           bool auto_login);
    Q_INVOKABLE void loginByEmailPassword(const QString& email,
                                          const QString& password,
                                          bool auto_login);
    Q_INVOKABLE void loginByMobileVerifyCode(const QString& phone_number,
                                             const QString& area_code,
                                             const QString& verify_code,
                                             bool auto_login);
    Q_SIGNAL void loginSuccessed();
    Q_SIGNAL void loginFailed(const QVariant& message);

    Q_INVOKABLE void logout();
    Q_SIGNAL void logouted();

    Q_INVOKABLE void resetPassWordWebsitebyPhone();
    Q_INVOKABLE void resetPassWordWebsitebyMail();
    Q_INVOKABLE void signupWebsitebyPhone();
    Q_INVOKABLE void signupWebsitebyMail();

   protected:
    auto startAccountLoginRequest(QPointer<AccountLoginRequest> request, bool auto_login) -> void;
    auto loadUserDetailInfo(const UserBaseInfo& base_info) -> void;

   private:
    QPointer<QrcodeImageProvider> qrcode_image_provider_;
    std::unique_ptr<QrcodeLoginLoopRequest> qrcode_login_request_;

    UserBaseInfo base_info_;
    UserDetailInfo detail_info_;
  };

}  // namespace cxcloud

#endif  // CXCLOUD_SERVICE_ACCOUNT_SERVICE_H_
