#include "cxcloud/net/account_request.h"

namespace cxcloud {

AccountLoginRequest::AccountLoginRequest(const QString& account,
                                         const QString& password,
                                         int type,
                                         QObject* parent)
    : HttpRequest(parent), account_(account), password_(password), type_(type) {}

UserBaseInfo AccountLoginRequest::getBaseInfo() const {
  return base_info_;
}

QByteArray AccountLoginRequest::getRequestData() const {
  return QJsonDocument{ {
    { QStringLiteral("type"), type_ },
    { QStringLiteral("account"), account_ },
    { QStringLiteral("password"), password_ },
  } }.toJson(QJsonDocument::JsonFormat::Compact);
}

void AccountLoginRequest::callWhenSuccessed() {
  auto result = getResponseJson().value(QStringLiteral("result")).toObject();
  base_info_.token = result.value(QStringLiteral("token")).toString();
  base_info_.user_id = result.value(QStringLiteral("userId")).toString();
}

QString AccountLoginRequest::getUrlTail() const {
  return QStringLiteral("/api/account/loginV2");
}

EmailLoginRequest::EmailLoginRequest(const QString& email,
                                     const QString& password,
                                     QObject* parent)
    : AccountLoginRequest(email, password, 2) {}  // 2-邮箱

PhoneLoginRequest::PhoneLoginRequest(const QString& phone_number,
                                     const QString& area_code,
                                     const QString& password,
                                     QObject* parent)
    : AccountLoginRequest(area_code + phone_number, password, 1) {}  // 1-手机号





LoadVerificationCodeRequest::LoadVerificationCodeRequest(const QString& phone_number,
                                                         const QString& area_code,
                                                         QObject* parent)
    : HttpRequest(parent), phone_number_(phone_number), area_code_(area_code) {}

QByteArray LoadVerificationCodeRequest::getRequestData() const {
  return QJsonDocument{ {
    { QStringLiteral("account"), area_code_ + phone_number_ },
    { QStringLiteral("verifyCodeType"), 6 },  // 6-快捷登录
    { QStringLiteral("accountType"), 1 },  // 1-手机
    { QStringLiteral("phoneAreaCode"), area_code_ },
  } }.toJson(QJsonDocument::JsonFormat::Compact);
}

QString LoadVerificationCodeRequest::getUrlTail() const {
  return QStringLiteral("/api/cxy/account/v2/getVerifyCode");
}





VerificationCodeLoginRequest::VerificationCodeLoginRequest(const QString& phone_number,
                                                           const QString& area_code,
                                                           const QString& verify_code,
                                                           QObject* parent)
    : HttpRequest(parent)
    , phone_number_(phone_number)
    , area_code_(area_code)
    , verify_code_(verify_code) {}

UserBaseInfo VerificationCodeLoginRequest::getBaseInfo() const {
  return base_info_;
}

QByteArray VerificationCodeLoginRequest::getRequestData() const {
  return QJsonDocument{ {
    { QStringLiteral("phoneNumber"), area_code_ + phone_number_ },
    { QStringLiteral("phoneAreaCode"), area_code_ },
    { QStringLiteral("verifyCode"), verify_code_ },
  } }.toJson(QJsonDocument::JsonFormat::Compact);
}

void VerificationCodeLoginRequest::callWhenSuccessed() {
  auto result = getResponseJson()[QStringLiteral("result")];
  base_info_.token = result[QStringLiteral("token")].toString();
  base_info_.user_id = result[QStringLiteral("userId")].toString();
}

QString VerificationCodeLoginRequest::getUrlTail() const {
  return QStringLiteral("/api/account/quickLogin");
}





QrcodeLoginLoopRequest::QrcodeLoginLoopRequest(
    std::function<void(const QString&)> url_updater,
    std::function<void(const cxcloud::UserBaseInfo&)> userinfo_updater,
    QObject* parent)
    : HttpRequest(parent)
    , working_(false)
    , qrimage_refresher_(url_updater)
    , userinfo_refresher_(userinfo_updater) {}

bool QrcodeLoginLoopRequest::isWorking() const {
  return working_;
}

void QrcodeLoginLoopRequest::startWorking() {
  working_ = true;
  setIdentical(QStringLiteral(""));
  HttpPost(this);
}

void QrcodeLoginLoopRequest::stopWorking() {
  setIdentical(QStringLiteral(""));
  working_ = false;
}

QByteArray QrcodeLoginLoopRequest::getRequestData() const {
  QJsonObject root;
  if (!identical_.isEmpty()) {
    root[QStringLiteral("identical")] = identical_;
  }
  return QJsonDocument{std::move(root)}.toJson(QJsonDocument::JsonFormat::Compact);
}

QString QrcodeLoginLoopRequest::getUrlTail() const {
  if (identical_.isEmpty()) {
    return QStringLiteral("/api/cxy/account/v2/qrLogin");
  }

  return QStringLiteral("/api/cxy/account/v2/qrQuery");
}

bool QrcodeLoginLoopRequest::isAutoDelete() const {
  return false;
}

void QrcodeLoginLoopRequest::callWhenSuccessed() {
  auto const result = getResponseJson()[QStringLiteral("result")];
  auto const identical = result[QStringLiteral("identical")];
  auto const state = result[QStringLiteral("state")];

  if (!identical.isUndefined()) {
    setIdentical(identical.toString());
  } else if (!state.isUndefined()) {
    switch (static_cast<ScanStatus>(state.toInt())) {
      case ScanStatus::WAITING:
      case ScanStatus::SCANNED:
        break;
      case ScanStatus::LOGINED:
        userinfo_refresher_({
          result[QStringLiteral("token")].toString(),
          QString::number(static_cast<size_t>(result[QStringLiteral("userId")].toDouble())),
        });
      case ScanStatus::EXPIRED:
      case ScanStatus::CANCELED:
      default:
        setIdentical(QStringLiteral(""));
        break;
    }
  }

  if (isWorking()) {
    using namespace std::chrono;
    QTimer::singleShot(500ms, this, [this]() { HttpPost(this); });
  }
}

void QrcodeLoginLoopRequest::setIdentical(const QString& identical) {
  identical_ = identical;

  if (!identical_.isEmpty()) {
    url_ = QStringLiteral("https://share.creality.com/scan-code?i=%1").arg(identical_);
  } else {
    url_.clear();
  }

  qrimage_refresher_(url_);
}





LoadUserDetailInfoRequest::LoadUserDetailInfoRequest(const UserBaseInfo& base_info,
                                                     QObject* parent)
    : HttpRequest(parent), base_info_(base_info) {}

UserBaseInfo LoadUserDetailInfoRequest::getBaseInfo() const {
  return base_info_;
}

UserDetailInfo LoadUserDetailInfoRequest::getDetailInfo() const {
  return detail_info_;
}

QString LoadUserDetailInfoRequest::getAccountUid() const {
  if (base_info_.user_id.isEmpty()) {
    return HttpRequest::getAccountUid();
  }

  return base_info_.user_id;
}

QString LoadUserDetailInfoRequest::getAccountToken() const {
  if (base_info_.token.isEmpty()) {
    return HttpRequest::getAccountToken();
  }

  return base_info_.token;
}

void LoadUserDetailInfoRequest::callWhenSuccessed() {
  auto result = getResponseJson()[QStringLiteral("result")];
  auto user_info = result[QStringLiteral("userInfo")];
  auto user_info_base = user_info[QStringLiteral("base")];

  detail_info_.avatar = user_info_base[QStringLiteral("avatar")].toString();
  detail_info_.nick_name = user_info_base[QStringLiteral("nickName")].toString();
  detail_info_.max_storage_space = user_info[QStringLiteral("maxStorageSpace")].toDouble();
  detail_info_.used_storage_space = user_info[QStringLiteral("usedStorageSpace")].toDouble();
}

QByteArray LoadUserDetailInfoRequest::getRequestData() const {
  return QStringLiteral("{}").toUtf8();
}

QString LoadUserDetailInfoRequest::getUrlTail() const {
  return QStringLiteral("/api/cxy/v2/user/getInfo");
}

}  // namespace cxcloud
