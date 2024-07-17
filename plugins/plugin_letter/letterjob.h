#pragma once
#include "qtusercore/module/job.h"
#include <QtGui/QMatrix4x4>
#include "cxkernel/wrapper/lettermodel.h"
#include "cxkernel/data/modelndata.h"

namespace trimesh
{
	class TriMesh;
};

namespace creative_kernel
{
	class ModelN;
};

class LetterJob : public qtuser_core::Job
{
	Q_OBJECT
signals:
	void finished();
public:
	LetterJob(QObject* parent = nullptr);
	virtual ~LetterJob();

	void SetModel(creative_kernel::ModelN* model);
	void SetObjects(const QList<QObject*>& objectList);
	void SetLetterModel(cxkernel::LetterModel* letterModel);
protected:
	QString name() override;
	QString description() override;
	void work(qtuser_core::Progressor* progressor) override;
	void failed() override;
	void successed(qtuser_core::Progressor* progressor) override;

	QList<QObject*> m_objectLists;
	cxkernel::LetterModel* m_letterModel;

	cxkernel::ModelNDataPtr m_newMeshdata;

	creative_kernel::ModelN* m_pModel;
	trimesh::TriMesh* m_pResultMesh;
};
