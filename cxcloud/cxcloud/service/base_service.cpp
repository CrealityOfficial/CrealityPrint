#include "cxcloud/service/base_service.h"

namespace cxcloud {

  BaseService::BaseService(std::weak_ptr<HttpSession> http_session, QObject* parent)
      : QObject{ parent }, http_session_{ http_session } {}

  auto BaseService::initialize() -> void {}

  auto BaseService::uninitialize() -> void {}

  auto BaseService::getHttpSession() const -> std::weak_ptr<HttpSession> {
    return http_session_;
  }

  auto BaseService::setHttpSession(std::weak_ptr<HttpSession> http_session) -> void {
    http_session_ = http_session;
  }

}  // namespace cxcloud
