#include "cadloadjob.h"

#include "qtusercore/module/progressortracer.h"
#include "qtusercore/string/resourcesfinder.h"

#include "stringutil/util.h"
#include "cxbin/load.h"

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
		
		std::wstring strWname = m_fileName.toStdWString();
		std::string strname = stringutil::wstring2string(strWname);
		trimesh::TriMesh* ms = Slic3r::load_step(strname.c_str(), &tracer);
		m_mesh = TriMeshPtr(ms);
	}
}
