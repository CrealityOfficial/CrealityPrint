#pragma once

#ifndef CXCLOUD_SERVICE_BASE_SERVICE_HPP_
#define CXCLOUD_SERVICE_BASE_SERVICE_HPP_

#include <memory>

#include <QtCore/QObject>

#include "cxcloud/interface.hpp"
#include "cxcloud/net/http_request.h"

namespace cxcloud {

class CXCLOUD_API BaseService : public QObject {
  Q_OBJECT;

  BaseService(BaseService&&) = delete;
  BaseService(const BaseService&) = delete;
  BaseService& operator=(BaseService&&) = delete;
  BaseService& operator=(const BaseService&) = delete;

public:
  explicit BaseService(std::weak_ptr<HttpSession> http_session, QObject* parent = nullptr)
      : QObject(parent), http_session_(http_session) {}
  virtual ~BaseService() = default;

public:
  virtual void initialize() {};
  virtual void uninitialize() {};

public:
  std::weak_ptr<HttpSession> getHttpSession() const { return http_session_; }
  void setHttpSession(std::weak_ptr<HttpSession> http_session) { http_session_ = http_session; }

  template<typename _Request, typename... _Args>
  _Request* createRequest(_Args&&... args) {
    if (http_session_.expired()) { return nullptr; }
    return http_session_.lock()->createRequest<_Request>(std::forward<_Args>(args)...);
  }

private:
  std::weak_ptr<HttpSession> http_session_;
};

}  // namespace cxcloud

#endif  // CXCLOUD_SERVICE_BASE_SERVICE_HPP_
