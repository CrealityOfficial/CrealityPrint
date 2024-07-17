#pragma once

#ifndef CXCLOUD_SERVICE_BASE_SERVICE_HPP_
#define CXCLOUD_SERVICE_BASE_SERVICE_HPP_

#include <memory>

#include <QtCore/QObject>

#include "cxcloud/interface.hpp"
#include "cxcloud/tool/http_component.h"

namespace cxcloud {

  class CXCLOUD_API BaseService : public QObject {
    Q_OBJECT;

    BaseService(BaseService&&) = delete;
    BaseService(const BaseService&) = delete;
    auto operator=(BaseService&&) -> BaseService& = delete;
    auto operator=(const BaseService&) -> BaseService& = delete;

   public:
    explicit BaseService(std::weak_ptr<HttpSession> http_session, QObject* parent = nullptr);
    virtual ~BaseService() = default;

   public:
    virtual auto initialize() -> void;
    virtual auto uninitialize() -> void;

   public:
    auto getHttpSession() const -> std::weak_ptr<HttpSession>;
    auto setHttpSession(std::weak_ptr<HttpSession> http_session) -> void;

    template<typename _Request, typename... _Args>
    auto createRequest(_Args&&... args) const -> QPointer<_Request> {
      if (http_session_.expired()) {
        return nullptr;
      }

      return http_session_.lock()->createRequest<_Request>(std::forward<_Args>(args)...);
    }

   private:
    std::weak_ptr<HttpSession> http_session_;
  };

}  // namespace cxcloud

#endif  // CXCLOUD_SERVICE_BASE_SERVICE_HPP_
