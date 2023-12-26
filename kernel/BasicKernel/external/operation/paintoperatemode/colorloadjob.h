#ifndef _NULLSPACE_COLOR_LOAD_JOB_1591235079966_H
#define _NULLSPACE_COLOR_LOAD_JOB_1591235079966_H

#include "cxkernel/data/header.h"
#include "qtusercore/module/job.h"
#include "trimesh2/TriMesh.h"

namespace spread
{
	class MeshSpreadWrapper;
	struct SceneData;
};

class ColorLoadJob : public qtuser_core::Job
{
	Q_OBJECT
signals:
	void sig_finished();

public:
	explicit ColorLoadJob(QObject* parent);
	virtual ~ColorLoadJob() = default;

	void setMeshSpreadWrapper(spread::MeshSpreadWrapper* wrapper);
	void setTriMesh(TriMeshPtr mesh);
	void setColorsList(const std::vector<trimesh::vec>& colorsList);
	void setData(const std::vector<std::string>& data);

	void wait();

private:
	spread::MeshSpreadWrapper* m_wrapper;
	TriMeshPtr m_mesh;
	std::vector<trimesh::vec> m_colorsList;
	std::vector<std::string> m_data;

	bool m_finished { false };

protected:
	virtual QString name() override;
	virtual QString description() override;
	virtual void work(qtuser_core::Progressor* progressor) override;
	virtual void failed() override;
	virtual void successed(qtuser_core::Progressor* progressor) override;
};

#endif // _NULLSPACE_COLOR_LOAD_JOB_1591235079966_H