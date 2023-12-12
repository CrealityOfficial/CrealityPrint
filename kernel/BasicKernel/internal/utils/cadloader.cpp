#include "cadloader.h"
#include "qtusercore/string/urlutils.h"
#include "cadloadjob.h"
//meshloadjob.h"
#include "cxkernel/utils/modelfrommeshjob.h"
#include "cxkernel/interface/jobsinterface.h"

namespace creative_kernel
{
	CADLoader::CADLoader(QObject* parent)
		: QObject(parent)
	{
	}

	CADLoader::~CADLoader()
	{

	}

	QString CADLoader::filter()
	{
		QString _filter = "CAD File (*.stp *.step)";
		return _filter;
	}

	void CADLoader::handle(const QString& fileName)
	{
		QStringList fileNames;
		fileNames << fileName;
		load(fileNames);
    }

	void CADLoader::handle(const QStringList& fileNames)
	{
		load(fileNames);
    }

	void CADLoader::load(const QStringList& fileNames)
	{
		QList<qtuser_core::JobPtr> jobs;
		for (const QString& fileName : fileNames)
		{
			CADLoadJob* loadJob = new CADLoadJob();
			loadJob->setFileName(fileName);
			jobs.push_back(qtuser_core::JobPtr(loadJob));
		}
		cxkernel:: executeJobs(jobs);


	}
}
