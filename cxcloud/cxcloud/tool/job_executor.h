#pragma once

#include <list>

#include <QtCore/QObject>
#include <QtCore/QPointer>

#include "cxcloud/interface.hpp"

// Adapters provided for qtuser_core::DetailDescription and qtuser_core::JobExecutor

namespace cxcloud {

  class Job;
  class JobExecutor;





  class CXCLOUD_API Job : public QObject {
    Q_OBJECT;

   public:
    explicit Job(QObject* parent = nullptr);
    virtual ~Job() override;

   public:  // emit by job owner
    Q_SIGNAL void started() const;
    Q_SIGNAL void finished() const;
    Q_SIGNAL void progressUpdated(float progress) const;
    Q_SIGNAL void messageUpdated(const QString& message) const;

   public:  // emit by executor
    Q_SIGNAL void interrupted() const;

   public:  // detail information
    Q_INVOKABLE QString get(const QString& key) const;
    Q_INVOKABLE void set(const QString& key, const QString& value);

   private:
    std::map<QString, QString> information_{};
  };





  class CXCLOUD_API JobExecutor : public QObject {
    Q_OBJECT;

   public:
    explicit JobExecutor(QObject* parent = nullptr);
    virtual ~JobExecutor() override;

   public:
    auto getCurrentJob() const -> QPointer<Job>;

    auto atatch(QPointer<Job> job) -> void;
    auto atatch(const std::list<QPointer<Job>>& job_list) -> void;
    auto detatch(QPointer<Job> job) -> void;
    auto detatch(const std::list<QPointer<Job>>& job_list) -> void;
    auto detatchAll() -> void;

   public:  // emit by executor
    Q_SIGNAL void jobsStart() const;
    Q_SIGNAL void jobsEnd() const;
    Q_SIGNAL void jobStart(QObject* job) const;
    Q_SIGNAL void jobEnd(QObject* job) const;
    Q_SIGNAL void jobProgress(float progress) const;
    Q_SIGNAL void jobMessage(const QString& message) const;

   public:  // qml to executor
    Q_INVOKABLE void stop();

   private:
    auto setCurrentJob(QPointer<Job> job) -> void;

    auto onJobFinished() -> void;

   private:
   QPointer<Job> current_job_{ nullptr };
    std::list<QPointer<Job>> job_list_{};
  };

}  // namespace cxcloud
