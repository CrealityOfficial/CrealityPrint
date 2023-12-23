#pragma once

#ifndef CXCLOUD_NET_HTTP_REQUEST_H_
#define CXCLOUD_NET_HTTP_REQUEST_H_

#include <atomic>
#include <functional>

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QObject>
#include <QtCore/QPointer>

#include <cpr/cpr.h>
#include <cpr/cprtypes.h>

#include "cxcloud/define.hpp"
#include "cxcloud/interface.hpp"
#include "cxcloud/tool/version.h"

#if (__cplusplus >= 201703L)
namespace boost::asio {
#else
namespace boost {
namespace asio {
#endif

class thread_pool;

#if (__cplusplus >= 201703L)
}  // namespace boost::asio
#else
}  // namespace asio
}  // namespace boost
#endif

namespace cxcloud {

class HttpSession;
class HttpRequest;


CXCLOUD_API void HttpGet(QPointer<HttpRequest> request);

CXCLOUD_API void HttpSyncPost(QPointer<HttpRequest> request);
CXCLOUD_API void HttpPost(QPointer<HttpRequest> request);

CXCLOUD_API void HttpSyncDownload(QPointer<HttpRequest> request);
CXCLOUD_API void HttpDownload(QPointer<HttpRequest> request);
CXCLOUD_API void HttpDownload(QPointer<HttpRequest> request, boost::asio::thread_pool& thread_pool);

class CXCLOUD_API HttpSession : public QObject {
  Q_OBJECT;

  HttpSession(HttpSession&&) = delete;
  HttpSession(const HttpSession&) = delete;
  HttpSession& operator=(HttpSession&&) = delete;
  HttpSession& operator=(const HttpSession&) = delete;

public:
  explicit HttpSession(Version application_version = { 0, 0, 0, 0 },
                       std::function<QString(void)> url_head_getter = nullptr,
                       std::function<QString(void)> account_uid_getter = nullptr,
                       std::function<QString(void)> account_token_getter = nullptr,
                       QObject* parent = nullptr);
  virtual ~HttpSession() = default;

public:
  Version getApplicationVersion() const;
  void setApplicationVersion(const Version& version);

  ApplicationType getApplicationType() const;
  void setApplicationType(ApplicationType type);

  std::function<QString(void)> getUrlHeadGetter() const;
  void setUrlHeadGetter(std::function<QString(void)>);

  std::function<QString(void)> getAccountUidGetter() const;
  void setAccountUidGetter(std::function<QString(void)>);

  std::function<QString(void)> getAccountTokenGetter() const;
  void setAccountTokenGetter(std::function<QString(void)>);

public:
  template<typename _Request, typename... _Args>
  _Request* createRequest(_Args&&... args) {
    auto* request = new _Request{ std::forward<_Args>(args)... };
    addRequest(request);
    return request;
  }

  template<typename _Request>
  void addRequest(_Request* request) {
    request->application_version_ = application_version_;
    request->application_type_ = application_type_;
    request->url_head_ = url_head_getter_ ? url_head_getter_() : QString{};
    request->account_uid_ = account_uid_getter_ ? account_uid_getter_() : QString{};
    request->account_token_ = account_token_getter_ ? account_token_getter_() : QString{};
  }

private:
  Version application_version_{ 0, 0, 0, 0 };
  ApplicationType application_type_{ ApplicationType::CREATIVE_PRINT };
  std::function<QString(void)> url_head_getter_{ nullptr };
  std::function<QString(void)> account_uid_getter_{ nullptr };
  std::function<QString(void)> account_token_getter_{ nullptr };
};

class CXCLOUD_API HttpRequest : public QObject {
  Q_OBJECT;

  HttpRequest(HttpRequest&&) = delete;
  HttpRequest(const HttpRequest&) = delete;
  HttpRequest& operator=(HttpRequest&&) = delete;
  HttpRequest& operator=(const HttpRequest&) = delete;

  friend CXCLOUD_API void HttpGet(QPointer<HttpRequest>);
  
  friend CXCLOUD_API void HttpSyncPost(QPointer<HttpRequest>);
  friend CXCLOUD_API void HttpPost(QPointer<HttpRequest>);
  
  friend CXCLOUD_API void HttpSyncDownload(QPointer<HttpRequest>);
  friend CXCLOUD_API void HttpDownload(QPointer<HttpRequest>);
  friend CXCLOUD_API void HttpDownload(QPointer<HttpRequest>, boost::asio::thread_pool&);

  friend class CXCLOUD_API HttpSession;

public:
  bool isInitlized();

public:
  using QObject::QObject;
  virtual ~HttpRequest() = default;

  virtual QByteArray getRequestData() const;
  virtual QString getDownloadSrcUrl() const;
  virtual QString getDownloadDstPath() const;

  virtual Version getApplicationVersion() const;
  virtual ApplicationType getApplicationType() const;
  virtual QString getAccountUid() const;
  virtual QString getAccountToken() const;
  virtual cpr::Header getHeader() const;

  void setParameters(cpr::Parameters Parameters);
  cpr::Parameters getParameters() const;

  virtual QString getUrlHead() const;
  virtual QString getUrlTail() const;
  virtual QString getUrl() const;

  size_t getTotalProgress() const;
  size_t getCurrentProgress() const;
  bool isCancelDownloadLater() const;
  void setCancelDownloadLater(bool cancel);

  const QByteArray& getResponseData() const;
  const QJsonObject& getResponseJson() const;
  cpr::ErrorCode getResponseState() const;
  const QString& getResponseMessage() const;

public:
  void setResponsedCallback(std::function<void(void)> callback);
  void setSuccessedCallback(std::function<void(void)> callback);
  void setFailedCallback(std::function<void(void)> callback);
  void setErredCallback(std::function<void(void)> callback);
  void setProgressUpdatedCallback(std::function<void(void)> callback);

protected:
  virtual bool isAutoDelete() const;

  virtual bool handleResponseData();

  void setTotalProgress(size_t progress);
  void setCurrentProgress(size_t progress);

  void setResponseData(const QByteArray& data);
  void setResponseJson(const QJsonObject& json);
  void setResponseState(const cpr::ErrorCode& state);
  void setResponseMessage(const QString& message);

protected:
  virtual void callWhenResponsed();
  virtual void callWhenSuccessed();
  virtual void callWhenFailed();
  virtual void callWhenErred();
  virtual void callWhenProgressUpdated();

  void callAfterResponsed();
  void callAfterSuccessed();
  void callAfterFailed();
  void callAfterErred();
  void callAfterProgressUpdated();

private:
  Version application_version_{ 0, 0, 0, 0 };
  ApplicationType application_type_{ ApplicationType::CREATIVE_PRINT };
  QString url_head_{};
  QString account_uid_{};
  QString account_token_{};

  mutable cpr::Header header_{};
  cpr::Parameters parameters_{};
  std::future<void> future_{};
  cpr::Response response_{};

  std::atomic<bool> cancel_download_later_{ false };
  size_t total_progress_{ 100 };
  size_t current_progress_{ 0 };

  QByteArray response_data_{};
  QJsonObject response_json_{};
  cpr::ErrorCode response_state_{};
  QString response_message_{};

  std::function<void(void)> after_responsed_callback_{ nullptr };
  std::function<void(void)> after_successed_callback_{ nullptr };
  std::function<void(void)> after_failed_callback_{ nullptr };
  std::function<void(void)> after_erred_callback_{ nullptr };
  std::function<void(void)> after_progress_updated_callback_{ nullptr };
};

}  // namespace cxcloud

#endif  // CXCLOUD_NET_HTTP_REQUEST_H_
