#pragma once
#include "data/modeln.h"
#include "qtusercore/module/job.h"

struct WrapDrillParam
{
	int cylinder_resolution;
	double cylinder_radius;
	double cylinder_depth;                         	// depth ??????????? 0 ?????????h?????????? 0???????????????????
	trimesh::vec3 cylinder_startPos;          //  ??????????????
	trimesh::vec3 cylinder_Dir;
	float bottomOffset;
};

class DrillJob : public qtuser_core::Job {
  Q_OBJECT;

public:
  explicit DrillJob(QObject* parent = nullptr);
  virtual ~DrillJob() = default;

public:
  void setModel(creative_kernel::ModelN* model);

public:
  void setParam(const WrapDrillParam& param);

protected:
  virtual QString name() override;
  virtual QString description() override;

  virtual void work(qtuser_core::Progressor* progressor) override;
  virtual void successed(qtuser_core::Progressor* progressor) override;
  virtual void failed() override;

private:
  creative_kernel::ModelN* model_;
  WrapDrillParam param_;
  
  creative_kernel::ModelNPropertyMeshChange change;
};
