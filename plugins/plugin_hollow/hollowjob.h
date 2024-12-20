#ifndef HOLLOWJOB_H_
#define HOLLOWJOB_H_

#include <list>
#include <tuple>

#include "data/header.h"
#include "data/modeln.h"
#include "qtusercore/module/job.h"

class HollowJob : public qtuser_core::Job {
  Q_OBJECT
public:
  HollowJob(QObject* parent = nullptr);
  virtual ~HollowJob();

public:
  void appendModel(creative_kernel::ModelN* model);

  Q_SIGNAL void finished(bool successed);

public:
  float getTihckness() const;
  void setThickness(float thickness);

  bool isFillEnabled() const;
  void setFillEnabled(bool fill);

  float getFillRatio() const;
  void setFillRatio(float ratio);

protected:
  QString name() override;
  QString description() override;

  void work(qtuser_core::Progressor* progressor) override;
  void successed(qtuser_core::Progressor* progressor) override;
  void failed() override;

private:
  using model_ptr_t = creative_kernel::ModelN*;
  using odata_ptr_t = cxkernel::MeshDataPtr;
  using task_cache_t = std::tuple<model_ptr_t, odata_ptr_t>;
  std::list<task_cache_t> task_cache_list_;

  float thickness_;
  bool fill_enabled_;
  float fill_ratio_;

public:
  static const float FILL_RATIO_MIN;
  static const float FILL_RATIO_MAX;
};

#endif  // HOLLOWJOB_H_