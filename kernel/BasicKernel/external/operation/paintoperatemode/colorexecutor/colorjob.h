#ifndef _NULLSPACE_COLOR_JOB_1591235079966_H
#define _NULLSPACE_COLOR_JOB_1591235079966_H

#include "qtusercore/module/job.h"

class ColorExecutor;

class ColorJob : public qtuser_core::Job
{
  Q_OBJECT
signals: 
	void sig_oneFinished(std::vector<int> dirtyChunks);
	void sig_allFinished();

public:
	explicit ColorJob(ColorExecutor* executor);
	virtual ~ColorJob() = default;

	virtual bool showProgress() override;

private:
	ColorExecutor* m_executor;

protected:
	virtual QString name() override;
	virtual QString description() override;
	virtual void work(qtuser_core::Progressor* progressor) override;
	virtual void failed() override;
	virtual void successed(qtuser_core::Progressor* progressor) override;
};

#endif // _NULLSPACE_COLOR_JOB_1591235079966_H