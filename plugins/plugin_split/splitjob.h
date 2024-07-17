#ifndef _NULLSPACE_SPLITJOB_1591256424823_H
#define _NULLSPACE_SPLITJOB_1591256424823_H
#include <QtCore/QObject>
#include "qtusercore/module/job.h"
#include "qtuser3d/math/plane.h"

#include "trimesh2/TriMesh.h"

namespace creative_kernel
{
	class ModelN;
}

class SplitJob: public qtuser_core::Job
{
public:
	SplitJob(QObject* parent = nullptr);
	virtual ~SplitJob();

	void setModel(creative_kernel::ModelN* model);
	void setPlane(const trimesh::vec3& position, const trimesh::vec3& normal);
	void setGap(float gap);
protected:
	QString name() override;
	QString description() override;
    void work(qtuser_core::Progressor* progressor) override;
    void failed() override; 
    void successed(qtuser_core::Progressor* progressor) override;
protected:
	float m_gap;
	creative_kernel::ModelN* m_model;
	trimesh::vec3 m_position;
	trimesh::vec3 m_normal;

	std::vector<trimesh::TriMesh*> m_meshes;
};
#endif // _NULLSPACE_SPLITJOB_1591256424823_H
