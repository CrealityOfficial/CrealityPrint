#ifndef _ExtendMeshGenJOB_1595582868614_H
#define _ExtendMeshGenJOB_1595582868614_H
#include "qtusercore/module/job.h"
#include "cxkernel/wrapper/photomeshmodel.h"

class Photo2MeshJob : public qtuser_core::Job
{
public:
	Photo2MeshJob(QObject* parent = nullptr);
	virtual ~Photo2MeshJob();

	void setFileName(const QString& fileName);
	void setModel(cxkernel::PhotoMeshModel* model);
protected:
	QString name() override;
	QString description() override;
	void work(qtuser_core::Progressor* progressor) override;
	void failed() override;
	void successed(qtuser_core::Progressor* progressor) override;
private:
	cxkernel::PhotoMeshModel* m_model;
	QString m_name;

	cxkernel::ModelNDataPtr m_data;
};

#endif // _ExtendMeshGenJOB_1595582868614_H