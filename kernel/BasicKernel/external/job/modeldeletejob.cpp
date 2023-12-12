#include "modeldeletejob.h"

namespace creative_kernel
{
	ModelDeleteJob::ModelDeleteJob(QObject* parent)
		: Job(parent)
		, m_model(nullptr)
	{
	}

	ModelDeleteJob::~ModelDeleteJob()
	{
	}

	void ModelDeleteJob::setModel(ModelN* model)
	{
		m_model = model;
	}

	QString ModelDeleteJob::name()
	{
		return "ModelDeleteJob";
	}

	QString ModelDeleteJob::description()
	{
		return "ModelDeleteJob.";
	}

	void ModelDeleteJob::failed()
	{
	}

	void ModelDeleteJob::successed(qtuser_core::Progressor* progressor)
	{
		if (m_model)
			delete m_model;
	}

	void ModelDeleteJob::work(qtuser_core::Progressor* progressor)
	{
	}
}
