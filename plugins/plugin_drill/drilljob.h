#pragma once

#include "trimesh2/TriMesh.h"
#include "data/modeln.h"

#include "qtusercore/module/job.h"
#include "qtusercore/module/progressor.h"

struct WrapDrillParam
{
	int cylinder_resolution;
	double cylinder_radius;
	double cylinder_depth;                         	// depth ����ΪС�ڵ��� 0 ʱ�����ֻ��һ��ڣ������� 0�����ָ������ڵ����б�
	trimesh::vec3 cylinder_startPos;          //  ��������������
	trimesh::vec3 cylinder_Dir;
};

class DrillJob : public qtuser_core::Job {
  Q_OBJECT;

public:
  explicit DrillJob(QObject* parent = nullptr);
  virtual ~DrillJob() = default;

public:
  creative_kernel::ModelN* getModel() const;
  void setModel(creative_kernel::ModelN* model);

public:
  void setParam(const WrapDrillParam& param);

public:
  Q_SIGNAL void finished(creative_kernel::ModelN* newModel);

protected:
  virtual QString name() override;
  virtual QString description() override;

  virtual void work(qtuser_core::Progressor* progressor) override;
  virtual void successed(qtuser_core::Progressor* progressor) override;
  virtual void failed() override;

private:
  creative_kernel::ModelN* model_;
  WrapDrillParam param_;
  
  creative_kernel::TriMeshPtr imesh_;

  cxkernel::ModelNDataPtr data_;
};
