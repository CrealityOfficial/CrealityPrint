#include "cxcloud/service/base_service.h"

namespace cxcloud {

  BaseService::BaseService(std::weak_ptr<HttpSession> http_session, QObject* parent)
      : QObject{ parent }, http_session_{ http_session } {}

  BaseService::BaseService(std::weak_ptr<HttpSession> http_session,
                           std::weak_ptr<JobExecutor> job_executor,
                           QObject*                   parent)
      : QObject{ parent }, http_session_{ http_session }, job_executor_{ job_executor } {}

  BaseService::BaseService(std::weak_ptr<HttpSession> http_session,
                           QPointer<QJSEngine>        js_engine,
                           QObject*                   parent)
      : QObject{ parent }, http_session_{ http_session }, js_engine_{ js_engine } {}

  BaseService::BaseService(std::weak_ptr<HttpSession> http_session,
                           std::weak_ptr<JobExecutor> job_executor,
                           QPointer<QJSEngine>        js_engine,
                           QObject*                   parent)
      : QObject{ parent }
      , http_session_{ http_session }
      , job_executor_{ job_executor }
      , js_engine_{ js_engine } {}

  auto BaseService::initialize() -> void {}

  auto BaseService::uninitialize() -> void {}

  auto BaseService::getHttpSession() const -> std::weak_ptr<HttpSession> {
    return http_session_;
  }

  auto BaseService::setHttpSession(std::weak_ptr<HttpSession> http_session) -> void {
    http_session_ = http_session;
  }

  auto BaseService::getJobExecutor() const -> std::weak_ptr<JobExecutor> {
    return job_executor_;
  }

  auto BaseService::setJobExecutor(std::weak_ptr<JobExecutor> job_executor) -> void {
    job_executor_ = job_executor;
  }

  auto BaseService::atatchJob(QPointer<Job> job) -> void {
    if (!job || job_executor_.expired()) {
      return;
    }

    job_executor_.lock()->atatch(job);
  }

  auto BaseService::detatchJob(QPointer<Job> job) -> void {
    if (!job || job_executor_.expired()) {
      return;
    }

    job_executor_.lock()->detatch(job);
  }

  auto BaseService::getJsEngine() const -> QPointer<QJSEngine> {
    return js_engine_;
  }

  auto BaseService::setJsEngine(QPointer<QJSEngine> engine) -> void {
    js_engine_ = engine;
  }

}  // namespace cxcloud
