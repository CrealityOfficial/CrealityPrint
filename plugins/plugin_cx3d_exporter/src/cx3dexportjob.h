#ifndef _CREATIVE_EXPORT_JOB_1590379536521_H
#define _CREATIVE_EXPORT_JOB_1590379536521_H
#include "qtusercore/module/job.h"
#include <QtCore/QSharedPointer>
#include <QtCore/QThread>
#include "trimesh2/TriMesh.h"
#include "cx3dwriter.h"
#include "cx3dscene.h"

namespace creative_kernel
{
    class ModelSpace;
}



class Cx3dScene;
class  Cx3dExportJob: public qtuser_core::Job
{
    Q_OBJECT

	public:
		enum JobType {WRITEJOB,READJOB};
		Cx3dExportJob(QString saveFilename,JobType jobtype, QObject* parent = nullptr);
		~Cx3dExportJob();

    protected:
		void work(qtuser_core::Progressor* progressor) override;
		void failed() override;                        // invoke from main thread
		void successed(qtuser_core::Progressor* progressor) override;
		void setMachineProfile();
		QString name() override;
    private:
		void AddModel2Scence();
    private:
		QString m_saveFilename;
		JobType m_jobType;
		Cx3dScene* m_buildScene;
		QThread* m_mainThread;

public:
	Q_INVOKABLE QString getMessageText();
	};

#endif // _CREATIVE_KERNEL_JOB_1590379536521_H
