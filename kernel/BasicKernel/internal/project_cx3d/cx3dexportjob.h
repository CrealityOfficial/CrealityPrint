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
		Cx3dExportJob(QString saveFilename, JobType jobtype, QObject* parent = nullptr);
		virtual ~Cx3dExportJob();

		void setCloseWindow(bool close);
		void setCaches(const QList<creative_kernel::ModelN*>& models);
		void setExportSuccessObject(QObject* object);
		void setShowProgress(bool show);
		void setNewBuild(bool needBuild);
		void setRemoveProjectCache(bool cache);
    protected:
		void work(qtuser_core::Progressor* progressor) override;
		void failed() override;                        // invoke from main thread
		void successed(qtuser_core::Progressor* progressor) override;
		void setMachineProfile();
		QString name() override;
		bool showProgress() override;
    private:
		void AddModel2Scence();
		
    private:
		QString m_saveFilename;
		JobType m_jobType;
		Cx3dScene* m_buildScene;
		QThread* m_mainThread;

		bool m_closeWindow;
		QList<creative_kernel::ModelN*> m_caches;
		QObject* m_successObject;

		bool m_showProgress;
		bool m_newBuild = false;
		bool m_removeProjectCache = true;
	public:
		Q_INVOKABLE QString getMessageText();
};

void saveProject(const QString& projectName, bool showProgress = true, QObject* successObject = nullptr, bool force = true, bool newBuild = false,bool removeProjectCache = true);
void loadProject(const QString& projectName,bool bCleanTemp=true);
#endif // _CREATIVE_KERNEL_JOB_1590379536521_H
