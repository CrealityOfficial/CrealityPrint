#ifndef _RepairJOB_1595582868614_H
#define _RepairJOB_1595582868614_H
#include "data/header.h"
#include "qtusercore/module/job.h"
#include "data/modeln.h"
#ifdef Q_OS_WINDOWS
class RepairJob : public qtuser_core::Job
{
	Q_OBJECT
public:
	RepairJob(QObject* parent = nullptr);
	virtual ~RepairJob();

	void setObject(QObject* receiver);
	void setModel(creative_kernel::ModelN* model);
protected:
	QString name() override;
	QString description() override;
	void work(qtuser_core::Progressor* progressor) override;
	void failed() override;
	void successed(qtuser_core::Progressor* progressor) override;
	
	trimesh::TriMesh* repairMeshObj(trimesh::TriMesh* choosedMesh, qtuser_core::Progressor* progressor);
private:
	creative_kernel::ModelN* m_model;
	cxkernel::ModelNDataPtr m_data;

	QObject* m_receiver;
};
#endif
#endif // _RepairJOB_1595582868614_H