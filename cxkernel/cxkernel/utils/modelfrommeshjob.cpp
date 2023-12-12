#include "modelfrommeshjob.h"

#include <QtCore/QDebug>

#include "qtusercore/module/progressortracer.h"

namespace cxkernel
{
	ModelFromMeshJob::ModelFromMeshJob(ModelNDataProcessor* processor, QObject* parent)
		: Job(parent)
		, m_processor(processor)
	{
	}

	ModelFromMeshJob::~ModelFromMeshJob()
	{
	}

	void ModelFromMeshJob::setInput(ModelCreateInput input)
	{
		m_input = input;
	}

	void ModelFromMeshJob::setParam(const ModelNDataCreateParam& param)
	{
		m_param = param;
	}

	QString ModelFromMeshJob::name()
	{
		return "ModelFromMeshJob";
	}

	QString ModelFromMeshJob::description()
	{
		return "ModelFromMeshJob";
	}

	void ModelFromMeshJob::failed()
	{
		qDebug() << "ModelFromMeshJob::failed";
	}

	void ModelFromMeshJob::successed(qtuser_core::Progressor* progressor)
	{
		if (m_data && m_processor)
			m_processor->process(m_data);
	}

	void ModelFromMeshJob::work(qtuser_core::Progressor* progressor)
	{
		qtuser_core::ProgressorTracer tracer(progressor);
		m_data = createModelNData(m_input, &tracer, m_param);
	}
}
