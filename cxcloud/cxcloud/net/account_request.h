#pragma once

#ifndef CXCLOUD_NET_ACCOUNT_REQUEST_H_
#define CXCLOUD_NET_ACCOUNT_REQUEST_H_

#include <atomic>
#include <functional>

#include "cxcloud/define.h"
#include "cxcloud/interface.hpp"
#include "cxcloud/tool/http_component.h"

namespace cxcloud {

  class CXCLOUD_API AccountLoginRequest : public HttpRequest {
    Q_OBJECT;

   public:
    enum class Type : int {
      PHONE = 1,
      EMAIL = 2,
    };

   public:
    explicit AccountLoginRequest(const QString& account,
                                 const QString& password,
                                 Type type,
                                 QObject* parent = nullptr);
    virtual ~AccountLoginRequest() override = default;

   public:
    auto getBaseInfo() const -> UserBaseInfo;

   protected:
    auto getRequestData() const -> QByteArray override;
    auto callWhenSuccessed() -> void override;
    auto getUrlTail() const -> QString override;

   protected:
    QString account_{};
    QString password_{};
    Type type_{ Type::EMAIL };

    UserBaseInfo base_info_{};
  };

  class CXCLOUD_API EmailLoginRequest : public AccountLoginRequest {
    Q_OBJECT;

   public:
    explicit EmailLoginRequest(const QString& email,
                               const QString& password,
                               QObject* parent = nullptr);
    virtual ~EmailLoginRequest() override = default;
  };

  class CXCLOUD_API PhoneLoginRequest : public AccountLoginRequest {
    Q_OBJECT;

   public:
    explicit PhoneLoginRequest(const QString& phone_number,
                               const QString& area_code,
                               const QString& password,
                               QObject* parent = nullptr);
    virtual ~PhoneLoginRequest() override = default;
  };





  class CXCLOUD_API LoadVerificationCodeRequest : public HttpRequest {
    Q_OBJECT;

   public:
    explicit LoadVerificationCodeRequest(const QString& phone_number,
                                         const QString& area_code,
                                         QObject* parent = nullptr);
    virtual ~LoadVerificationCodeRequest() override = default;

   protected:
    auto getRequestData() const -> QByteArray override;
    auto getUrlTail() const -> QString override;

   protected:
    QString phone_number_{};
    QString area_code_{};
  };

  class CXCLOUD_API VerificationCodeLoginRequest : public HttpRequest {
    Q_OBJECT;

   public:
    explicit VerificationCodeLoginRequest(const QString& phone_number,
                                          const QString& area_code,
                                          const QString& verify_code,
                                          QObject* parent = nullptr);
    virtual ~VerificationCodeLoginRequest() override = default;

   public:
    auto getBaseInfo() const -> UserBaseInfo;

   protected:
    auto getRequestData() const -> QByteArray override;
    auto callWhenSuccessed() -> void override;
    auto getUrlTail() const -> QString override;

   private:
    QString phone_number_{};
    QString verify_code_{};
    QString area_code_{};
    UserBaseInfo base_info_{};
  };





  class CXCLOUD_API QrcodeLoginLoopRequest : public HttpRequest {
    Q_OBJECT;

   public:
    explicit QrcodeLoginLoopRequest(std::function<void(const QString&)> url_updater,
                                    std::function<void(const UserBaseInfo&)> userinfo_updater,
                                    QObject* parent = nullptr);
    virtual ~QrcodeLoginLoopRequest() override = default;

   public:
    auto isWorking() const -> bool;
    auto startWorking() -> void;
    auto stopWorking() -> void;

   protected:
    auto getRequestData() const -> QByteArray override;
    auto getUrlTail() const -> QString override;
    auto isAutoDelete() const -> bool override;
    auto callWhenSuccessed() -> void override;
    auto callWhenFailed() -> void override;

   protected:
    auto setIdentical(const QString& identical) -> void;
    auto tryAsyncPost() -> void;

   private:
    enum class ScanStatus : int {
      WAITING = 1,
      SCANNED = 2,
      LOGINED = 3,
      EXPIRED = 4,
      CANCELED = 5,
    };

    std::atomic<bool> working_{ false };
    QString identical_{};
    QString url_{};

    std::function<void(const QString&)> qrimage_refresher_{ nullptr };
    std::function<void(const UserBaseInfo&)> userinfo_refresher_{ nullptr };
  };





  class CXCLOUD_API LoadUserDetailInfoRequest : public HttpRequest {
    Q_OBJECT;

   public:
    explicit LoadUserDetailInfoRequest(const UserBaseInfo& base_info = {},
                                       QObject* parent = nullptr);
    virtual ~LoadUserDetailInfoRequest() override = default;

   public:
    auto getBaseInfo() const -> UserBaseInfo;
    auto getDetailInfo() const -> UserDetailInfo;

   public:
    auto getAccountUid() const -> QString override;
    auto getAccountToken() const -> QString override;

   protected:
    auto callWhenSuccessed() -> void override;
    auto getRequestData() const -> QByteArray override;
    auto getUrlTail() const -> QString override;

   private:
    UserBaseInfo base_info_{};
    UserDetailInfo detail_info_{};
  };





  class CXCLOUD_API LoadPrivacySettingsRequest : public HttpRequest {
    Q_OBJECT;

   public:
    explicit LoadPrivacySettingsRequest(QObject* parent = nullptr);
    virtual ~LoadPrivacySettingsRequest() override = default;

   public:
    auto getModelRestriction() const -> ModelRestriction;
    auto isModelCollectionAsPrivate() const -> bool;

   protected:
    auto getUrlTail() const -> QString override;
    auto getRequestData() const -> QByteArray override;
    auto callWhenSuccessed() -> void override;

   private:
    ModelRestriction model_restriction_{ DEFAULT_MODEL_RESTRICTION };
    bool model_collection_as_private_{ false };
  };

}  // namespace cxcloud

#endif  // CXCLOUD_NET_ACCOUNT_REQUEST_H_
