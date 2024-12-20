#include "modeldeletejob.h"
#include "data/modeln.h"
#include "data/modelgroup.h"

namespace creative_kernel
{
	ModelDeleteJob::ModelDeleteJob(QObject* parent)
		: Job(parent)
		, m_modelGroup(nullptr)
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

	void ModelDeleteJob::setModelGroup(ModelGroup* modelGroup)
	{
		m_modelGroup = modelGroup;
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
		if (m_modelGroup)
			delete m_modelGroup;
	}

	void ModelDeleteJob::work(qtuser_core::Progressor* progressor)
	{
		//creative_kernel::ModelGroup* group = qobject_cast<creative_kernel::ModelGroup*>(m_model->parent());
		
	}
}
