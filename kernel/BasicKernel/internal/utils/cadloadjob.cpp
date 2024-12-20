#include "cadloadjob.h"

#include "qtusercore/module/progressortracer.h"
#include "qtusercore/string/resourcesfinder.h"
#include "interface/spaceinterface.h"

#include "cxkernel/utils/utils.h"
#include "cadcore/step/STEP.hpp"

namespace creative_kernel
{
	CADLoadJob::CADLoadJob(QObject* parent)
		: Job(parent)
		, m_processor(nullptr)
	{
	}

	CADLoadJob::~CADLoadJob()
	{
	}

	void CADLoadJob::setFileName(const QString& fileName)
	{
		m_fileName = fileName;
	}

	void CADLoadJob::setModelNDataProcessor(cxkernel::ModelNDataProcessor* processor)
	{
		m_processor = processor;
	}

	QString CADLoadJob::name()
	{
		return "Load Mesh";
	}

	QString CADLoadJob::description()
	{
		return "Load Mesh.";
	}

	void CADLoadJob::failed()
	{
		qDebug() << "CADLoadJob::failed.";
	}

	void CADLoadJob::successed(qtuser_core::Progressor* progressor)
	{
		if (m_processor)
			m_processor->process(m_data);
	}

	void CADLoadJob::work(qtuser_core::Progressor* progressor)
	{
		qtuser_core::ProgressorTracer tracer(progressor);
		
		TriMeshPtr mesh(cadcore::load_step(cxkernel::qString2String(m_fileName), &tracer));

		QString shortName = m_fileName;
		QStringList stringList = shortName.split("/");
		if (stringList.size() > 0)
			shortName = stringList.back();

		cxkernel::ModelCreateInput input;
		input.mesh = mesh;
		input.fileName = m_fileName;
		input.name = shortName;
		m_data = cxkernel::createModelNData(input);
	}
}
