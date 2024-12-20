#ifndef _NULLSPACE_MIRROR_JOB_1591235079966_H
#define _NULLSPACE_MIRROR_JOB_1591235079966_H

#include "basickernelexport.h"
#include "data/modeln.h"
#include "cxkernel/data/header.h"
#include "qtusercore/module/job.h"

class BASIC_KERNEL_API MirrorJob : public qtuser_core::Job
{
	Q_OBJECT
signals:
	void sig_finished();

public:
	explicit MirrorJob(const QList<creative_kernel::ModelN*>& models, int mode);

	virtual ~MirrorJob();

private:
	QList<creative_kernel::ModelN*> m_models;
	//QList<cxkernel::ModelNDataPtr> m_modelNDatas;
	QList<creative_kernel::SharedDataID> m_modelIDs;
  int m_mode;

protected:
	virtual QString name() { return "mirror"; }
	
  virtual QString description() { return ""; }
	
  virtual void work(qtuser_core::Progressor* progressor);

	virtual void failed();

	virtual void successed(qtuser_core::Progressor* progressor);

};

#endif // _NULLSPACE_MIRROR_JOB_1591235079966_H

