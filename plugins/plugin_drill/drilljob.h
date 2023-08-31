#pragma once

#include "trimesh2/TriMesh.h"
#include "data/modeln.h"

#include "mmesh/util/drill.h"

#include "qtusercore/module/job.h"
#include "qtusercore/module/progressor.h"

class DrillJob : public qtuser_core::Job {
  Q_OBJECT;

public:
  explicit DrillJob(QObject* parent = nullptr);
  virtual ~DrillJob() = default;

public:
  creative_kernel::ModelN* getModel() const;
  void setModel(creative_kernel::ModelN* model);

public:
  mmesh::DrillParam getParam() const;
  void setParam(const mmesh::DrillParam& param);

public:
  Q_SIGNAL void finished(bool successed);

protected:
  virtual QString name() override;
  virtual QString description() override;

  virtual void work(qtuser_core::Progressor* progressor) override;
  virtual void successed(qtuser_core::Progressor* progressor) override;
  virtual void failed() override;

private:
  creative_kernel::ModelN* model_;
  mmesh::DrillParam param_;
  
  creative_kernel::TriMeshPtr imesh_;
  creative_kernel::TriMeshPtr omesh_;
};
