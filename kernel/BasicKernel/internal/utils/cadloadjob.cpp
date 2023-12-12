#include "cadloadjob.h"

#include "qtusercore/module/progressortracer.h"
#include "qtusercore/string/resourcesfinder.h"

#include "cxkernel/utils/utils.h"

#include "cxkernel/interface/modelninterface.h"

#include "cadcore/step/STEP.hpp"
namespace creative_kernel
{
	CADLoadJob::CADLoadJob(QObject* parent)
		: Job(parent)
	{
	}

	CADLoadJob::~CADLoadJob()
	{
	}

	void CADLoadJob::setFileName(const QString& fileName)
	{
		m_fileName = fileName;
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
		QString shortName = m_fileName;
		QStringList stringList = shortName.split("/");
		if (stringList.size() > 0)
			shortName = stringList.back();

		cxkernel::ModelCreateInput input;
		input.mesh = m_mesh;
		input.fileName = m_fileName;
		input.name = shortName;
		input.type = cxkernel::ModelNDataType::mdt_file;
		addModelFromCreateInput(input);
	}

	void CADLoadJob::work(qtuser_core::Progressor* progressor)
	{
		qtuser_core::ProgressorTracer tracer(progressor);
		
		trimesh::TriMesh* ms = cadcore::load_step(cxkernel::qString2String(m_fileName), &tracer);
		m_mesh = TriMeshPtr(ms);
	}
}
