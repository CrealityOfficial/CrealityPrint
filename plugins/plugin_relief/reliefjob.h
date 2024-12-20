#pragma once

#include <qtusercore/module/job.h>

class ReliefJob : public qtuser_core::Job {
  Q_OBJECT;

 public:
  explicit ReliefJob(QObject* parent = nullptr);
  virtual ~ReliefJob() override = default;

 protected:
  auto name() -> QString override;
  auto description() -> QString override;

  auto work(qtuser_core::Progressor* progressor) -> void override;
  auto successed(qtuser_core::Progressor* progressor) -> void override;
  auto failed() -> void override;

 private:

};
