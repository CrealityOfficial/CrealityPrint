#ifndef _NULLSPACE_SPLITJOB_1591256424823_H
#define _NULLSPACE_SPLITJOB_1591256424823_H
#include <QtCore/QObject>
#include "qtusercore/module/job.h"

#include "data/modeln.h"
#include "data/modelgroup.h"

#include "cxkernel/data/meshdata.h"

struct CutParam
{
	bool cut2Parts;
	double gap;
	trimesh::vec3 position;
	trimesh::vec3 normal;
	bool isCutGroup;
};

class SplitJob: public qtuser_core::Job
{
	Q_OBJECT
public:
	SplitJob(QObject* parent = nullptr);
	virtual ~SplitJob();

	void setParts(const QList<creative_kernel::ModelN*>& models, creative_kernel::ModelGroup* modelGroup);
	void setModelGroups(const QList<creative_kernel::ModelGroup*>& modelGroups);

	void setCutParam(const CutParam& param);

	void prepareWork();

signals:
	void beforeModifyScene();
	void onSplitSuccess();

protected:
	bool isTriMeshAtPlaneForward(trimesh::TriMesh* mesh, trimesh::vec3 pos, trimesh::vec3 dir);
	QVector3D getOffset(bool isForward, trimesh::vec3 direction, float gap);
	QString name() override;
	QString description() override;
    void work(qtuser_core::Progressor* progressor) override;
    void failed() override; 
    void successed(qtuser_core::Progressor* progressor) override;

protected:
	/* operate parts in single group */
	QList<creative_kernel::ModelN*> m_models;
	creative_kernel::ModelGroup* m_modelGroup {NULL};

	/* operate groups */
	QList<creative_kernel::ModelGroup*> m_modelGroups;
	
	CutParam m_param;
	struct SplitResult
	{
		// creative_kernel::ModelGroup* modelGroup = NULL;
		creative_kernel::ModelN* model = NULL;
		//QList<cxkernel::MeshDataPtr> datas;
		QList<int> splitIDs; 
		QList<int> meshIDs;
		QList<bool> directions;
		QList<QVector3D> centers;
		QList<QVector3D> centerOffsets;;
	};
	QMap<creative_kernel::ModelGroup*, QList<SplitResult>> m_results;
};
#endif // _NULLSPACE_SPLITJOB_1591256424823_H
