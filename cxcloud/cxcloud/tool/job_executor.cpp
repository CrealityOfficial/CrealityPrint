#include "cxcloud/tool/job_executor.h"

#include <algorithm>

#include <QtQml/QQmlEngine>

namespace cxcloud {

  Job::Job(QObject* parent) : QObject{ parent } {
    QQmlEngine::setObjectOwnership(this, QQmlEngine::ObjectOwnership::CppOwnership);
  }

  Job::~Job() {
    finished();
  }

  auto Job::get(const QString& key) const -> QString {
    auto iter = information_.find(key);
    if (iter == information_.cend()) {
      return {};
    }

    return iter->second;
  }

  auto Job::set(const QString& key, const QString& value) -> void {
    information_.emplace(key, value);
  }





  JobExecutor::JobExecutor(QObject* parent) : QObject{ parent } {
    QQmlEngine::setObjectOwnership(this, QQmlEngine::ObjectOwnership::CppOwnership);
  }

  JobExecutor::~JobExecutor() {
    detatchAll();
  }

  auto JobExecutor::getCurrentJob() const -> QPointer<Job> {
    return current_job_;
  }

  auto JobExecutor::setCurrentJob(QPointer<Job> job) -> void {
    if (current_job_ == job) {
      return;
    }

    if (current_job_) {
      current_job_->disconnect(this);
    }

    current_job_ = std::move(job);
    if (current_job_) {
      connect(current_job_.data(), &Job::finished, this, &JobExecutor::onJobFinished);
      connect(current_job_.data(), &Job::interrupted, this, &JobExecutor::onJobFinished);
      connect(current_job_.data(), &Job::progressUpdated, this, &JobExecutor::jobProgress);
      connect(current_job_.data(), &Job::messageUpdated, this, &JobExecutor::jobMessage);
    }
  }

  auto JobExecutor::atatch(QPointer<Job> job) -> void {
    atatch(std::list<QPointer<Job>>{ job });
  }

  auto JobExecutor::atatch(const std::list<QPointer<Job>>& job_list) -> void {
    auto difference = decltype(job_list_){};
    std::set_difference(job_list.cbegin(), job_list.cend(),
                        job_list_.cbegin(), job_list_.cend(),
                        std::back_inserter(difference));

    job_list_.splice(job_list_.cend(), std::move(difference));

    if (auto job = getCurrentJob(); !job && !job_list_.empty()) {
      jobsStart();
      setCurrentJob(job_list_.front());
      jobStart(job_list_.front());
    }
  }

  auto JobExecutor::detatch(QPointer<Job> job) -> void {
    detatch(std::list<QPointer<Job>>{ job });
  }

  auto JobExecutor::detatch(const std::list<QPointer<Job>>& job_list) -> void {
    auto intersection = decltype(job_list_){};
    std::set_intersection(job_list.cbegin(), job_list.cend(),
                          job_list_.cbegin(), job_list_.cend(),
                          std::back_inserter(intersection));

    if (auto job = getCurrentJob(); job) {
      if (std::find(intersection.cbegin(), intersection.cend(), job) != intersection.cend()) {
        setCurrentJob(nullptr);
        jobEnd(job);
      }
    }

    job_list_.remove_if([this, &intersection](auto&& job) {
      return std::find(intersection.cbegin(), intersection.cend(), job) != job_list_.cend();
    });

    if (job_list_.empty()) {
      jobsEnd();
      return;
    }

    if (auto job = getCurrentJob(); !job) {
      setCurrentJob(job_list_.front());
    }
  }

  auto JobExecutor::detatchAll() -> void {
    detatch(job_list_);
  }

  auto JobExecutor::stop() -> void {
    if (auto job = getCurrentJob(); job) {
      job->interrupted();
    }
  }

  auto JobExecutor::onJobFinished() -> void {
    auto* job = sender();

    job_list_.erase(std::find(job_list_.cbegin(), job_list_.cend(), job));
    if (getCurrentJob() != job) {
      return;
    }

    setCurrentJob(nullptr);
    jobEnd(job);

    if (job_list_.empty()) {
      jobsEnd();
      return;
    }

    setCurrentJob(job_list_.front());
    jobStart(job_list_.front());
  }

}  // namespace cxcloud
