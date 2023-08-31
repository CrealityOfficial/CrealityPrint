#ifndef _SPLITPARTSJOB_1599194027546_H
#define _SPLITPARTSJOB_1599194027546_H
#include "qtusercore/module/job.h"

#include "trimesh2/TriMesh.h"

namespace creative_kernel
{
	class ModelN;
}

namespace qtuser_core
{
	class Progressor;
}

class SplitPartsJob : public qtuser_core::Job
{
public:
	SplitPartsJob(QObject* parent = nullptr);
	virtual ~SplitPartsJob();
	
	void setModel(creative_kernel::ModelN* model);
protected:
	QString name() override;
	QString description() override;
    void work(qtuser_core::Progressor* progressor) override;
    void failed() override; 
    void successed(qtuser_core::Progressor* progressor) override;
	
protected:
	creative_kernel::ModelN* m_model;

	std::vector<trimesh::TriMesh*> m_meshes;
};

#endif // _SPLITPARTSJOB_1599194027546_H