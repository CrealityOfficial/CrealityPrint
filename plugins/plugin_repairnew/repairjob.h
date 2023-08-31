#ifndef _RepairJOB_1595582868614_H
#define _RepairJOB_1595582868614_H
#include "data/header.h"
#include "qtusercore/module/job.h"
#include "qquaternion.h"
#include "qvector3d.h"
#include <QTimer>

using namespace trimesh;


namespace creative_kernel
{
	class ModelN;
}

class RepairJob : public qtuser_core::Job
{
	Q_OBJECT
public:
	RepairJob(QObject* parent = nullptr);
	virtual ~RepairJob();

	void setObject(QObject* receiver);
	qtuser_core::Progressor* progressor;
	void setModel(creative_kernel::ModelN* model);
	trimesh::TriMesh*  repairMeshObj(trimesh::TriMesh * choosedMesh, qtuser_core::Progressor* progressor);

protected:
	QString name() override;
	QString description() override;
	void work(qtuser_core::Progressor* progressor) override;
	void failed() override;
	void successed(qtuser_core::Progressor* progressor) override;
	
signals:
	void Repaired();
private:
	creative_kernel::ModelN* m_model;
	trimesh::TriMesh* m_mesh;
	creative_kernel::TriMeshPtr m_repaireddMesh;
	QObject* m_receiver;

};
#if 0
class JudgeModelJob : public qtuser_core::Job
{
	Q_OBJECT
public:
	JudgeModelJob(QObject* parent = nullptr);
	virtual ~JudgeModelJob();
	qtuser_core::Progressor* progressor;
	void setModel(creative_kernel::ModelN* model);
	bool judgeModelBoundaries(trimesh::TriMesh* choosedMesh, qtuser_core::Progressor* progressor);

protected:
	QString name() override;
	QString description() override;
	void work(qtuser_core::Progressor* progressor) override;
	void failed() override;
	void successed(qtuser_core::Progressor* progressor) override;

signals:
	void JudgeModelFinished();
private:
	creative_kernel::ModelN* m_model;
	bool m_hasBoundaries;
};
#endif

#endif // _RepairJOB_1595582868614_H