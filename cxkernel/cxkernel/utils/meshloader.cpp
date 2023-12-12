#include "meshloader.h"
#include "qtusercore/string/urlutils.h"

#include "cxkernel/utils/meshloadjob.h"
#include "cxkernel/utils/modelfrommeshjob.h"
#include "cxkernel/interface/jobsinterface.h"

namespace cxkernel
{
	MeshLoader::MeshLoader(QObject* parent)
		: QObject(parent)
		, m_processor(nullptr)
	{
	}

	MeshLoader::~MeshLoader()
	{

	}

	QString MeshLoader::filter()
	{
		QString _filter = "Mesh File (*.stl *.obj *.dae *.3mf *.3ds *.wrl *.cxbin *.off *.ply)";
		return _filter;
	}

	void MeshLoader::handle(const QString& fileName)
	{
		QStringList fileNames;
		fileNames << fileName;
		load(fileNames);
    }

	void MeshLoader::handle(const QStringList& fileNames)
	{
		load(fileNames);
    }

	void MeshLoader::setParam(const ModelNDataCreateParam& param)
	{
		m_param = param;
	}

	void MeshLoader::load(const QStringList& fileNames)
	{
		QList<qtuser_core::JobPtr> jobs;
		for (const QString& fileName : fileNames)
		{
			MeshLoadJob* loadJob = new MeshLoadJob();
			loadJob->setFileName(fileName);
			jobs.push_back(qtuser_core::JobPtr(loadJob));
		}
		executeJobs(jobs);
	}

	void MeshLoader::addModelFromCreateInput(const ModelCreateInput& input)
	{
		if (m_processor)
		{
			ModelFromMeshJob* job = new ModelFromMeshJob(m_processor);
			job->setParam(m_param);
			job->setInput(input);
			executeJob(qtuser_core::JobPtr(job), true);
		}
	}

	void MeshLoader::setModelNDataProcessor(ModelNDataProcessor* processor)
	{
		m_processor = processor;
	}
}
