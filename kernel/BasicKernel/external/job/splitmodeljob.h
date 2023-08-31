#ifndef _CREATIVE_KERNEL_SPLIT_MODEL_JOB_H
#define _CREATIVE_KERNEL_SPLIT_MODEL_JOB_H
#include "basickernelexport.h"
#include "qtusercore/module/job.h"
#include "trimesh2/TriMesh.h"
#include "qvector3d.h"
#include "qquaternion.h"


namespace creative_kernel
{
	class ModelN;
}

namespace qtuser_core
{
	class Progressor;
}

struct SplitModelInfor
{
	QString objectName;
	QVector3D localPos;
	QVector3D scale;
	QQuaternion qtRotate;
};

typedef std::shared_ptr<trimesh::TriMesh> TriMeshPointer;

class BASIC_KERNEL_API SplitModelJob : public qtuser_core::Job
{
public:
	SplitModelJob(QObject* parent = nullptr);
	virtual ~SplitModelJob();

	void EnableSplit(bool bSplit);

	void setModel(QList<creative_kernel::ModelN*> model);

protected:
	QString name();
	QString description();
	void failed();                        // invoke from main thread
	void successed(qtuser_core::Progressor* progressor);                     // invoke from main thread
	void work(qtuser_core::Progressor* progressor);    // invoke from worker thread


public:
	void SplitModel(qtuser_core::Progressor* progressor);
	void AddSplitModel(qtuser_core::Progressor* progressor);
	void MergeModel(qtuser_core::Progressor* progressor);


private:
	bool m_bSplit;
	QList<creative_kernel::ModelN*> m_models;
	std::vector<std::vector<TriMeshPointer>> m_meshes;
	SplitModelInfor m_SplitModelInfor;
	TriMeshPointer m_pnewMesh = NULL;

};

#endif // _CREATIVE_KERNEL_MESHLOADJOB_1590984808257_H
