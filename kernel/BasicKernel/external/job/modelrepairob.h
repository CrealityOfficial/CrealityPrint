#ifndef _NULLSPACE_SPLITJOB_1591256424823_H
#define _NULLSPACE_SPLITJOB_1591256424823_H
#include <QtCore/QObject>
#include "basickernelexport.h"
#include "qtusercore/module/job.h"

#include "data/modeln.h"
#include "data/modelgroup.h"
#include "cxkernel/data/meshdata.h"

namespace creative_kernel
{
	class BASIC_KERNEL_API ModelRepairWinJob : public qtuser_core::Job
	{
		Q_OBJECT

	public:
		ModelRepairWinJob(QObject* parent = nullptr);
		virtual ~ModelRepairWinJob();

		void setParts(const QList<creative_kernel::ModelN*>& models, creative_kernel::ModelGroup* modelGroup);
		void setModelGroups(const QList<creative_kernel::ModelGroup*>& modelGroups);

	protected:
		QString name() override;
		QString description() override;
		void work(qtuser_core::Progressor* progressor) override;
		void failed() override;
		void successed(qtuser_core::Progressor* progressor) override;

	signals:
		void repairSuccessed();

	protected:
		QList<cxkernel::MeshDataPtr> m_meshDatas;
	};
}
#endif // _NULLSPACE_SPLITJOB_1591256424823_H
