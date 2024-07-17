#pragma once

#ifndef CXCLOUD_TOOL_HTTP_COMPONENT_H_
#define CXCLOUD_TOOL_HTTP_COMPONENT_H_

#include <atomic>
#include <functional>
#include <future>

#include <QtCore/QJsonObject>
#include <QtCore/QObject>
#include <QtCore/QPointer>

#include "cxcloud/define.h"
#include "cxcloud/interface.hpp"
#include "cxcloud/tool/thread_pool.h"
#include "cxcloud/tool/version.h"

namespace cxcloud {

  class CXCLOUD_API HttpRequest;
  class CXCLOUD_API HttpSession;

  enum class HttpErrorCode {
    OK = 0,
    CONNECTION_FAILURE,
    EMPTY_RESPONSE,
    HOST_RESOLUTION_FAILURE,
    INTERNAL_ERROR,
    INVALID_URL_FORMAT,
    NETWORK_RECEIVE_ERROR,
    NETWORK_SEND_FAILURE,
    OPERATION_TIMEDOUT,
    PROXY_RESOLUTION_FAILURE,
    SSL_CONNECT_ERROR,
    SSL_LOCAL_CERTIFICATE_ERROR,
    SSL_REMOTE_CERTIFICATE_ERROR,
    SSL_CACERT_ERROR,
    GENERIC_SSL_ERROR,
    UNSUPPORTED_PROTOCOL,
    REQUEST_CANCELLED,
    TOO_MANY_REDIRECTS,
    UNKNOWN_ERROR = 1000,
  };

  using HttpHeader = std::map<QString, QString>;
  using HttpParameters = std::map<QString, QString>;





  class CXCLOUD_API HttpRequest : public QObject {
    Q_OBJECT;

    HttpRequest(HttpRequest&&) = delete;
    HttpRequest(const HttpRequest&) = delete;
    auto operator=(HttpRequest&&) -> HttpRequest& = delete;
    auto operator=(const HttpRequest&) -> HttpRequest& = delete;

    friend class CXCLOUD_API HttpSession;

   public:
    explicit HttpRequest(QObject* parent = nullptr);
    virtual ~HttpRequest() = default;

   public:
    auto isInitlized() -> bool;

    virtual auto getRequestData() const -> QByteArray;
    virtual auto getDownloadSrcUrl() const -> QString;
    virtual auto getDownloadDstPath() const -> QString;

    virtual auto getApplicationVersion() const -> Version;
    virtual auto getApplicationType() const -> ApplicationType;
    virtual auto getAccountUid() const -> QString;
    virtual auto getAccountToken() const -> QString;
    virtual auto getHeader() const -> HttpHeader;
    virtual auto getParameters() const -> HttpParameters;

    virtual auto getUrlHead() const -> QString;
    virtual auto getUrlTail() const -> QString;
    virtual auto getUrl() const -> QString;

    auto getTryTimes() const -> size_t;
    auto setTryTimes(size_t times) -> void;

    auto getTotalProgress() const -> size_t;
    auto getCurrentProgress() const -> size_t;
    auto isCancelDownloadLater() const -> bool;
    auto setCancelDownloadLater(bool cancel) -> void;

    auto getResponseData() const -> const QByteArray&;
    auto getResponseJson() const -> const QJsonObject&;
    auto getResponseState() const -> HttpErrorCode;
    auto getResponseMessage() const -> const QString&;

   public:
    auto setResponsedCallback(std::function<void(void)> callback) -> void;
    auto setSuccessedCallback(std::function<void(void)> callback) -> void;
    auto setFailedCallback(std::function<void(void)> callback) -> void;
    auto setErredCallback(std::function<void(void)> callback) -> void;
    auto setProgressUpdatedCallback(std::function<void(void)> callback) -> void;

   public:
    auto syncGet() -> void;
    auto asyncGet() -> void;
    auto asyncGet(ThreadPool& pool) -> void;

    auto syncPost() -> void;
    auto asyncPost() -> void;
    auto asyncPost(ThreadPool& pool) -> void;

    auto syncDownload() -> void;
    auto asyncDownload() -> void;
    auto asyncDownload(ThreadPool& pool) -> void;

    auto isAsyncTasking() const -> bool;

   protected:
    virtual auto isAutoDelete() const -> bool;

    virtual auto handleResponseData() -> bool;

    auto setTotalProgress(size_t progress) -> void;
    auto setCurrentProgress(size_t progress) -> void;

    auto setResponseData(const QByteArray& data) -> void;
    auto setResponseJson(const QJsonObject& json) -> void;
    auto setResponseState(const HttpErrorCode& state) -> void;
    auto setResponseMessage(const QString& message) -> void;

   protected:
    virtual auto callWhenResponsed() -> void;
    virtual auto callWhenSuccessed() -> void;
    virtual auto callWhenFailed() -> void;
    virtual auto callWhenErred() -> void;
    virtual auto callWhenProgressUpdated() -> void;

    auto callAfterResponsed() -> void;
    auto callAfterSuccessed() -> void;
    auto callAfterFailed() -> void;
    auto callAfterErred() -> void;
    auto callAfterProgressUpdated() -> void;

   private:
    Version application_version_{ 0, 0, 0, 0 };
    ApplicationType application_type_{ ApplicationType::CREALITY_PRINT };
    QString url_head_{};
    QString account_uid_{};
    QString account_token_{};

    mutable HttpHeader header_{};
    HttpParameters parameters_{};
    std::future<void> future_{};
    size_t try_times_{ 3 };

    std::atomic<bool> cancel_download_later_{ false };
    size_t total_progress_{ 100 };
    size_t current_progress_{ 0 };

    QByteArray response_data_{};
    QJsonObject response_json_{};
    HttpErrorCode response_state_{};
    QString response_message_{};

    std::function<void(void)> after_responsed_callback_{ nullptr };
    std::function<void(void)> after_successed_callback_{ nullptr };
    std::function<void(void)> after_failed_callback_{ nullptr };
    std::function<void(void)> after_erred_callback_{ nullptr };
    std::function<void(void)> after_progress_updated_callback_{ nullptr };
  };





  class CXCLOUD_API HttpSession : public QObject {
    Q_OBJECT;

    HttpSession(HttpSession&&) = delete;
    HttpSession(const HttpSession&) = delete;
    auto operator=(HttpSession&&) -> HttpSession& = delete;
    auto operator=(const HttpSession&) -> HttpSession& = delete;

   public:
    explicit HttpSession(Version application_version = {},
                         std::function<QString(void)> url_head_getter = nullptr,
                         std::function<QString(void)> account_uid_getter = nullptr,
                         std::function<QString(void)> account_token_getter = nullptr,
                         QObject* parent = nullptr);
    virtual ~HttpSession() = default;

   public:
    auto getApplicationVersion() const -> Version;
    auto setApplicationVersion(const Version& version) -> void;

    auto getApplicationType() const -> ApplicationType;
    auto setApplicationType(ApplicationType type) -> void;

    auto getUrlHeadGetter() const -> std::function<QString(void)>;
    auto setUrlHeadGetter(std::function<QString(void)> getter) -> void;

    auto getAccountUidGetter() const -> std::function<QString(void)>;
    auto setAccountUidGetter(std::function<QString(void)> getter) -> void;

    auto getAccountTokenGetter() const -> std::function<QString(void)>;
    auto setAccountTokenGetter(std::function<QString(void)> getter) -> void;

   public:
    template<typename _Request, typename... _Args>
    auto createRequest(_Args&&... args) -> QPointer<_Request> {
      auto* request = new _Request{ std::forward<_Args>(args)... };
      addRequest(request);
      return request;
    }

    auto addRequest(QPointer<HttpRequest> request) -> void;

   private:
    Version application_version_{ 0, 0, 0, 0 };
    ApplicationType application_type_{ ApplicationType::CREALITY_PRINT };
    std::function<QString(void)> url_head_getter_{ nullptr };
    std::function<QString(void)> account_uid_getter_{ nullptr };
    std::function<QString(void)> account_token_getter_{ nullptr };
  };

}  // namespace cxcloud

#endif  // CXCLOUD_TOOL_HTTP_COMPONENT_H_
