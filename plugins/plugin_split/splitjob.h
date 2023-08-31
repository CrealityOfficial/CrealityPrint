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
	void setPlane(const qtuser_3d::Plane& plane);
protected:
	QString name() override;
	QString description() override;
    void work(qtuser_core::Progressor* progressor) override;
    void failed() override; 
    void successed(qtuser_core::Progressor* progressor) override;
protected:
	creative_kernel::ModelN* m_model;
	qtuser_3d::Plane m_plane;

	std::vector<trimesh::TriMesh*> m_meshes;
};
#endif // _NULLSPACE_SPLITJOB_1591256424823_H
