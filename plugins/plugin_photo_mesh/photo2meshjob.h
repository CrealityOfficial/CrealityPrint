#ifndef _ExtendMeshGenJOB_1595582868614_H
#define _ExtendMeshGenJOB_1595582868614_H
#include "qtusercore/module/job.h"
#include "data/header.h"

struct Photo2MeshInput
{
	QString fileName;
	int blur;
	int colorSeg;
	int edgeType;
	int convertType;

	float baseHeight;
	float maxHeight;
	int index;

	bool invert;
	float meshXWidth;
	int pointXNum;
	int pointYNum;

	Photo2MeshInput()
	{
		blur = 9;
		edgeType = 0;
		colorSeg = -1;
		convertType = 0;
		baseHeight = 0.35f;
		maxHeight = 2.2f;
		index = 0;
		invert = true;
		meshXWidth = 140.0f;
		pointXNum = 1000;
		pointYNum = 1000;
	}
};

class Photo2MeshJob : public qtuser_core::Job
{
public:
	Photo2MeshJob(QObject* parent = nullptr);
	virtual ~Photo2MeshJob();

	void setInput(const Photo2MeshInput& input);
protected:
	QString name() override;
	QString description() override;
	void work(qtuser_core::Progressor* progressor) override;
	void failed() override;
	void successed(qtuser_core::Progressor* progressor) override;

	creative_kernel::TriMeshPtr createMeshFromInput(qtuser_core::Progressor* progressor);
private:
	Photo2MeshInput m_input;
	creative_kernel::TriMeshPtr m_mesh;
};

#endif // _ExtendMeshGenJOB_1595582868614_H