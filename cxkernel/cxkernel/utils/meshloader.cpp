#include "meshloader.h"
#include "qtusercore/string/urlutils.h"

#include "cxkernel/utils/meshloadjob.h"
#include "cxkernel/utils/modelfrommeshjob.h"
#include "cxkernel/interface/jobsinterface.h"
#include <QFileInfo>
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
		QString _filter = "Mesh File (*.stl *.obj *.dae *.3ds *.wrl *.off *.ply *.3mf)";
		return _filter;
	}

	void MeshLoader::handle(const QString& fileName)
	{
		if (!m_jobs.empty())
		{
			m_tasks.emplace_back(fileName);
			return;
		}

		load(QStringList{ fileName });
    }

	void MeshLoader::handle(const QStringList& fileNames)
	{
		if (!m_jobs.empty())
		{
			m_tasks.emplace_back(fileNames);
			return;
		}

		load(fileNames);
    }

	void MeshLoader::setParam(const ModelNDataCreateParam& param)
	{
		m_param = param;
	}

	void MeshLoader::load(const QStringList& fileNames)
	{
		QList<qtuser_core::JobPtr> jobs;
		int jobCount=0;
		for (const QString& fileName : fileNames)
		{
			MeshLoadJob* loadJob = new MeshLoadJob();
			loadJob->setModelNDataProcessor(m_processor);
			loadJob->setFileName(fileName);
			loadJob->attachObserver(this);
			jobs.push_back(qtuser_core::JobPtr(loadJob));
			m_jobs.emplace(loadJob);
			QFileInfo info(fileName);
			if(info.suffix().toLower()!="3mf")
			{
				jobCount++;
			}
		}
		if (m_processor)
			m_processor->modelMeshLoadStarted(jobCount);
		executeJobs(jobs);
	}

	void MeshLoader::addModelFromCreateInput(const ModelCreateInput& input)
	{
		if (m_processor)
		{
			ModelFromMeshJob* job = new ModelFromMeshJob(m_processor);
			job->setParam(m_param);
			job->setInput(input);
			job->attachObserver(this);
			m_jobs.emplace(job);
			executeJob(qtuser_core::JobPtr(job), true);
		}
	}

	void MeshLoader::setModelNDataProcessor(ModelNDataProcessor* processor)
	{
		m_processor = processor;
	}

	void MeshLoader::onFinished(MeshJob* job) {
		m_jobs.erase(job);
		if (m_jobs.empty() && !m_tasks.empty()) {
			auto files = std::move(m_tasks.front());
			m_tasks.pop_front();
			load(files);
		}
	}
}
