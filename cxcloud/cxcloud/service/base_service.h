#pragma once

#ifndef CXCLOUD_SERVICE_BASE_SERVICE_HPP_
#define CXCLOUD_SERVICE_BASE_SERVICE_HPP_

#include <memory>

#include <QtCore/QObject>
#include <QtQml/QJSEngine>

#include "cxcloud/interface.hpp"
#include "cxcloud/tool/http_component.h"
#include "cxcloud/tool/job_executor.h"

namespace cxcloud {

  class CXCLOUD_API BaseService : public QObject {
    Q_OBJECT;

    BaseService(BaseService&&) = delete;
    BaseService(const BaseService&) = delete;
    auto operator=(BaseService&&) -> BaseService& = delete;
    auto operator=(const BaseService&) -> BaseService& = delete;

   public:
    explicit BaseService(std::weak_ptr<HttpSession> http_session, QObject* parent = nullptr);
    explicit BaseService(std::weak_ptr<HttpSession> http_session,
                         std::weak_ptr<JobExecutor> job_executor,
                         QObject*                   parent = nullptr);
    explicit BaseService(std::weak_ptr<HttpSession> http_session,
                         QPointer<QJSEngine>        js_engine,
                         QObject*                   parent = nullptr);
    explicit BaseService(std::weak_ptr<HttpSession> http_session,
                         std::weak_ptr<JobExecutor> job_executor,
                         QPointer<QJSEngine>        js_engine,
                         QObject*                   parent = nullptr);
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

    auto getJobExecutor() const -> std::weak_ptr<JobExecutor>;
    auto setJobExecutor(std::weak_ptr<JobExecutor> job_executor) -> void;
    auto atatchJob(QPointer<Job> job) -> void;
    auto detatchJob(QPointer<Job> job) -> void;

    auto getJsEngine() const -> QPointer<QJSEngine>;
    auto setJsEngine(QPointer<QJSEngine> engine) -> void;

   private:
    std::weak_ptr<HttpSession> http_session_{};
    std::weak_ptr<JobExecutor> job_executor_{};
    QPointer<QJSEngine> js_engine_{ nullptr };
  };

}  // namespace cxcloud

#endif  // CXCLOUD_SERVICE_BASE_SERVICE_HPP_
