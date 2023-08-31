#pragma once
#include "qtusercore/module/job.h"
#include <QtGui/QMatrix4x4>

namespace trimesh
{
	class TriMesh;
};

namespace creative_kernel
{
	class ModelN;
};

class LetterOp;

class LetterJob : public qtuser_core::Job
{
public:
	LetterJob(LetterOp* theOp = nullptr, QObject* parent = nullptr);
	virtual ~LetterJob();

	void SetModel(creative_kernel::ModelN* model);
	void SetTextMeshs(std::vector<trimesh::TriMesh*>& theTextMeshs, std::vector<QMatrix4x4>& theTextMeshPoses);
	void SetIsTextOutside(bool value);

protected:
	QString name() override;
	QString description() override;
	void work(qtuser_core::Progressor* progressor) override;
	void failed() override;
	void successed(qtuser_core::Progressor* progressor) override;

	void TransformMesh(trimesh::TriMesh* original_mesh,  QMatrix4x4 pose, trimesh::TriMesh* transformed_mesh);

	creative_kernel::ModelN* m_pModel;
	std::vector<trimesh::TriMesh*> m_vTextMeshs;
	std::vector<QMatrix4x4> m_vTextMeshPoses;
	trimesh::TriMesh* m_pResultMesh;
	LetterOp* m_pOp;
	bool m_bIsTextOutside;  // true: relief text, false: intaglio text
};
