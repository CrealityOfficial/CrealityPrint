#include "cxcloud/net/account_request.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QTimer>

#include "cxcloud/tool/function.h"

namespace cxcloud {

  AccountLoginRequest::AccountLoginRequest(const QString& account,
                                           const QString& password,
                                           Type type,
                                           QObject* parent)
      : HttpRequest{ parent }, account_{ account }, password_{ password }, type_{ type } {}

  auto AccountLoginRequest::getBaseInfo() const -> UserBaseInfo {
    return base_info_;
  }

  auto AccountLoginRequest::getRequestData() const -> QByteArray {
    return QJsonDocument{ {
      { QStringLiteral("type"), static_cast<int>(type_) },
      { QStringLiteral("account"), account_ },
      { QStringLiteral("password"), password_ },
    } }.toJson(QJsonDocument::JsonFormat::Compact);
  }

  auto AccountLoginRequest::callWhenSuccessed() -> void {
    const auto result = responseJson()[QStringLiteral("result")];
    base_info_.token = result[QStringLiteral("token")].toString();

    auto user_info = result[QStringLiteral("userInfo")];
    if (!user_info.isObject()) {
      user_info = result[QStringLiteral("user_info")];
    }

    if (user_info.isObject()) {
      base_info_.user_id = user_info[QStringLiteral("userId")].toVariant().toString();
    }

    if (base_info_.user_id.isEmpty()) {
      base_info_.user_id = result[QStringLiteral("userId")].toVariant().toString();
    }
  }

  auto AccountLoginRequest::getUrlTail() const -> QString {
    return QStringLiteral("/api/cxy/account/v2/loginV2");
  }

  EmailLoginRequest::EmailLoginRequest(const QString& email,
                                       const QString& password,
                                       QObject* parent)
      : AccountLoginRequest{ email, password, Type::EMAIL } {}

  PhoneLoginRequest::PhoneLoginRequest(const QString& phone_number,
                                       const QString& area_code,
                                       const QString& password,
                                       QObject* parent)
      : AccountLoginRequest{ area_code + phone_number, password, Type::PHONE } {}





  LoadVerificationCodeRequest::LoadVerificationCodeRequest(const QString& phone_number,
                                                           const QString& area_code,
                                                           QObject* parent)
      : HttpRequest{ parent }, phone_number_{ phone_number }, area_code_{ area_code } {}

  auto LoadVerificationCodeRequest::getRequestData() const -> QByteArray {
    return QJsonDocument{ {
      { QStringLiteral("account"), area_code_ + phone_number_ },
      { QStringLiteral("verifyCodeType"), 6 },  // 6-快捷登录
      { QStringLiteral("accountType"), static_cast<int>(AccountLoginRequest::Type::PHONE) },
      { QStringLiteral("phoneAreaCode"), area_code_ },
    } }.toJson(QJsonDocument::JsonFormat::Compact);
  }

  auto LoadVerificationCodeRequest::getUrlTail() const -> QString {
    return QStringLiteral("/api/cxy/account/v2/getVerifyCode");
  }





  VerificationCodeLoginRequest::VerificationCodeLoginRequest(const QString& phone_number,
                                                             const QString& area_code,
                                                             const QString& verify_code,
                                                             QObject* parent)
      : HttpRequest{ parent }
      , phone_number_{ phone_number }
      , verify_code_{ verify_code }
      , area_code_{ area_code } {}

  auto VerificationCodeLoginRequest::getBaseInfo() const -> UserBaseInfo {
    return base_info_;
  }

  auto VerificationCodeLoginRequest::getRequestData() const -> QByteArray {
    return QJsonDocument{ {
      { QStringLiteral("phoneNumber"), area_code_ + phone_number_ },
      { QStringLiteral("phoneAreaCode"), area_code_ },
      { QStringLiteral("verifyCode"), verify_code_ },
    } }.toJson(QJsonDocument::JsonFormat::Compact);
  }

  auto VerificationCodeLoginRequest::callWhenSuccessed() -> void {
    const auto result = responseJson()[QStringLiteral("result")];
    base_info_.token = result[QStringLiteral("token")].toString();
    base_info_.user_id = result[QStringLiteral("userId")].toVariant().toString();
  }

  auto VerificationCodeLoginRequest::getUrlTail() const -> QString {
    return QStringLiteral("/api/account/quickLogin");
  }





  QrcodeLoginLoopRequest::QrcodeLoginLoopRequest(
    std::function<void(const QString&)> url_updater,
    std::function<void(const cxcloud::UserBaseInfo&)> userinfo_updater,
    QObject* parent)
      : HttpRequest{ parent }
      , working_{ false }
      , qrimage_refresher_{ url_updater }
      , userinfo_refresher_{ userinfo_updater } {}

  auto QrcodeLoginLoopRequest::isWorking() const -> bool {
    return working_;
  }

  auto QrcodeLoginLoopRequest::startWorking() -> void {
    working_ = true;
    tryAsyncPost();
  }

  auto QrcodeLoginLoopRequest::stopWorking() -> void {
    working_ = false;
  }

  auto QrcodeLoginLoopRequest::getRequestData() const -> QByteArray {
    return QJsonDocument{ QJsonObject{
      {
        QStringLiteral("identical"),
        [this] {
          if (identical_.isEmpty()) {
            return QJsonValue{ QJsonValue::Type::Undefined };
          } else {
            return QJsonValue::fromVariant(QVariant::fromValue(identical_));
          }
        }()
      },
    } }.toJson(QJsonDocument::JsonFormat::Compact);
  }

  auto QrcodeLoginLoopRequest::getUrlTail() const -> QString {
    if (identical_.isEmpty()) {
      return QStringLiteral("/api/cxy/account/v2/qrLogin");
    }

    return QStringLiteral("/api/cxy/account/v2/qrQuery");
  }

  auto QrcodeLoginLoopRequest::isAutoDelete() const -> bool {
    return false;
  }

  auto QrcodeLoginLoopRequest::callWhenSuccessed() -> void {
    const auto result = responseJson()[QStringLiteral("result")];

    // got qrcode
    if (identical_.isEmpty()) {
      if (const auto identical = result[QStringLiteral("identical")]; identical.isString()) {
        setIdentical(identical.toString());
      }
    }

    // checked qrcode
    else if (const auto state = result[QStringLiteral("state")]; state.isDouble()) {
      switch (static_cast<ScanStatus>(state.toInt())) {
        case ScanStatus::WAITING:
        case ScanStatus::SCANNED: {
          break;
        }
        case ScanStatus::LOGINED: {
          userinfo_refresher_({
            result[QStringLiteral("token")].toString(),
            QString::number(static_cast<size_t>(result[QStringLiteral("userId")].toDouble())),
          });
        }
        case ScanStatus::EXPIRED:
        case ScanStatus::CANCELED:
        default: {
          setIdentical(QStringLiteral(""));
          break;
        }
      }
    }

    using namespace std::chrono;
    QTimer::singleShot(identical_.isEmpty() ? 5s : 500ms, [this] {
      tryAsyncPost();
    });
  }

  auto QrcodeLoginLoopRequest::callWhenFailed() -> void {
    stopWorking();
    startWorking();
  }

  auto QrcodeLoginLoopRequest::setIdentical(const QString& identical) -> void {
    identical_ = identical;

    if (!identical_.isEmpty()) {
      url_ = QStringLiteral("https://share.creality.com/scan-code?i=%1").arg(identical_);
    } else {
      url_.clear();
    }

    qrimage_refresher_(url_);
  }

  auto QrcodeLoginLoopRequest::tryAsyncPost() -> void {
    if (isWorking() && !isAsyncTasking()) {
      asyncPost();
    }
  }





  LoadUserDetailInfoRequest::LoadUserDetailInfoRequest(const UserBaseInfo& base_info,
                                                       QObject* parent)
      : HttpRequest{ parent }, base_info_{ base_info } {}

  auto LoadUserDetailInfoRequest::getBaseInfo() const -> UserBaseInfo {
    return base_info_;
  }

  auto LoadUserDetailInfoRequest::getDetailInfo() const -> UserDetailInfo {
    return detail_info_;
  }

  auto LoadUserDetailInfoRequest::getAccountUid() const -> QString {
    if (base_info_.user_id.isEmpty()) {
      return HttpRequest::getAccountUid();
    }

    return base_info_.user_id;
  }

  auto LoadUserDetailInfoRequest::getAccountToken() const -> QString {
    if (base_info_.token.isEmpty()) {
      return HttpRequest::getAccountToken();
    }

    return base_info_.token;
  }

  auto LoadUserDetailInfoRequest::callWhenSuccessed() -> void {
    const auto result = responseJson()[QStringLiteral("result")];
    const auto user_info = result[QStringLiteral("userInfo")];
    const auto user_info_base = user_info[QStringLiteral("base")];

    detail_info_.avatar = user_info_base[QStringLiteral("avatar")].toString();
    detail_info_.nick_name = user_info_base[QStringLiteral("nickName")].toString();
    detail_info_.max_storage_space = user_info[QStringLiteral("maxStorageSpace")].toDouble();
    detail_info_.used_storage_space = user_info[QStringLiteral("usedStorageSpace")].toDouble();
  }

  auto LoadUserDetailInfoRequest::getRequestData() const -> QByteArray {
    return QStringLiteral("{}").toUtf8();
  }

  auto LoadUserDetailInfoRequest::getUrlTail() const -> QString {
    return QStringLiteral("/api/cxy/v2/user/getInfo");
  }





  LoadPrivacySettingsRequest::LoadPrivacySettingsRequest(QObject* parent) : HttpRequest{ parent } {}

  auto LoadPrivacySettingsRequest::getModelRestriction() const -> ModelRestriction {
    return model_restriction_;
  }

  auto LoadPrivacySettingsRequest::isModelCollectionAsPrivate() const -> bool {
    return model_collection_as_private_;
    ModelRestriction::IGNORE_;
  }

  auto LoadPrivacySettingsRequest::getUrlTail() const -> QString {
    return QStringLiteral("/api/cxy/v2/user/privacySettings/get");
  }

  auto LoadPrivacySettingsRequest::getRequestData() const -> QByteArray {
    return QByteArrayLiteral("{}");
  }

  auto LoadPrivacySettingsRequest::callWhenSuccessed() -> void {
    const auto result = responseJson()[QStringLiteral("result")];
    model_restriction_ =
        StringToModelRestriction(result[QStringLiteral("modelModeration")].toString());
    model_collection_as_private_ = result[QStringLiteral("modelCollectionPrivate")].toBool();
  }

}  // namespace cxcloud
