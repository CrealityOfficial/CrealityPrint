#pragma once
#include "qtusercore/module/job.h"
#include <QtGui/QMatrix4x4>
#include "cxkernel/wrapper/lettermodel.h"
#include "cxkernel/data/meshdata.h"


namespace trimesh
{
	class TriMesh;
};

namespace creative_kernel
{
	class ModelGroup;
};

class LetterJob : public qtuser_core::Job
{
	Q_OBJECT
signals:
	void finished();
public:
	LetterJob(QObject* parent = nullptr);
	virtual ~LetterJob();

	void SetModelGroup(creative_kernel::ModelGroup* modelgroup);
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

	QList<cxkernel::MeshDataPtr> m_meshDatas;

	creative_kernel::ModelGroup* m_pModelGroup;
};
