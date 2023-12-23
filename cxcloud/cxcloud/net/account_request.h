#pragma once

#ifndef CXCLOUD_NET_ACCOUNT_REQUEST_H_
#define CXCLOUD_NET_ACCOUNT_REQUEST_H_

#include <QtCore/QTimer>
#include <atomic>
#include <functional>

#include "cxcloud/define.hpp"
#include "cxcloud/interface.hpp"
#include "cxcloud/net/http_request.h"

namespace cxcloud {

class CXCLOUD_API AccountLoginRequest : public HttpRequest {
  Q_OBJECT;

public:
  explicit AccountLoginRequest(const QString& account,
                               const QString& password,
                               int type,
                               QObject* parent = nullptr);
  virtual ~AccountLoginRequest() = default;

public:
  UserBaseInfo getBaseInfo() const;

protected:
  QByteArray getRequestData() const override;
  void callWhenSuccessed() override;
  QString getUrlTail() const override;

protected:
  QString account_;
  QString password_;
  int type_;

  UserBaseInfo base_info_;
};

class CXCLOUD_API EmailLoginRequest : public AccountLoginRequest {
  Q_OBJECT;

public:
  explicit EmailLoginRequest(const QString& email,
                             const QString& password,
                             QObject* parent = nullptr);
  virtual ~EmailLoginRequest() = default;
};

class CXCLOUD_API PhoneLoginRequest : public AccountLoginRequest {
  Q_OBJECT;

public:
  explicit PhoneLoginRequest(const QString& phone_number,
                             const QString& area_code,
                             const QString& password,
                             QObject* parent = nullptr);
  virtual ~PhoneLoginRequest() = default;
};

class CXCLOUD_API LoadVerificationCodeRequest : public HttpRequest {
  Q_OBJECT;

public:
  explicit LoadVerificationCodeRequest(const QString& phone_number,
                                       const QString& area_code,
                                       QObject* parent = nullptr);
  virtual ~LoadVerificationCodeRequest() = default;

protected:
  QByteArray getRequestData() const override;
  QString getUrlTail() const override;

protected:
  QString phone_number_;
  QString area_code_;
};

class CXCLOUD_API VerificationCodeLoginRequest : public HttpRequest {
  Q_OBJECT;

public:
  explicit VerificationCodeLoginRequest(const QString& phone_number,
                                        const QString& area_code,
                                        const QString& verify_code,
                                        QObject* parent = nullptr);
  virtual ~VerificationCodeLoginRequest() = default;

public:
  UserBaseInfo getBaseInfo() const;

protected:
  QByteArray getRequestData() const override;
  void callWhenSuccessed() override;
  QString getUrlTail() const override;

private:
  QString phone_number_;
  QString verify_code_;
  QString area_code_;
  UserBaseInfo base_info_;
};

class CXCLOUD_API QrcodeLoginLoopRequest : public HttpRequest {
  Q_OBJECT;

public:
  explicit QrcodeLoginLoopRequest(std::function<void(const QString&)> url_updater,
                                  std::function<void(const UserBaseInfo&)> userinfo_updater,
                                  QObject* parent = nullptr);
  virtual ~QrcodeLoginLoopRequest() = default;

public:
  bool isWorking() const;
  void startWorking();
  void stopWorking();

protected:
  QByteArray getRequestData() const override;
  QString getUrlTail() const override;
  bool isAutoDelete() const override;
  void callWhenSuccessed() override;

protected:
  void setIdentical(const QString& identical);

private:
  enum class ScanStatus : int {
    WAITING = 1,
    SCANNED = 2,
    LOGINED = 3,
    EXPIRED = 4,
    CANCELED = 5,
  };

  std::atomic<bool> working_;
  QString identical_;
  QString url_;

  std::function<void(const QString&)> qrimage_refresher_;
  std::function<void(const UserBaseInfo&)> userinfo_refresher_;
};

class CXCLOUD_API LoadUserDetailInfoRequest : public HttpRequest {
  Q_OBJECT;

public:
  explicit LoadUserDetailInfoRequest(const UserBaseInfo& base_info = {},
                                     QObject* parent = nullptr);
  virtual ~LoadUserDetailInfoRequest() = default;

public:
  UserBaseInfo getBaseInfo() const;
  UserDetailInfo getDetailInfo() const;

public:
  QString getAccountUid() const override;
  QString getAccountToken() const override;

protected:
  void callWhenSuccessed() override;
  QByteArray getRequestData() const override;
  QString getUrlTail() const override;

private:
  UserBaseInfo base_info_;
  UserDetailInfo detail_info_;
};

}  // namespace cxcloud

#endif  // CXCLOUD_NET_ACCOUNT_REQUEST_H_
