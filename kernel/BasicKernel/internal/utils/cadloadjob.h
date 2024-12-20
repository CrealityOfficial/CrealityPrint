#ifndef _CREATIVE_KERNEL_CADLOADJOB_1590984808257_H
#define _CREATIVE_KERNEL_CADLOADJOB_1590984808257_H
#include "qtusercore/module/job.h"
#include "cxkernel/data/modelndata.h"

namespace creative_kernel
{
	class CADLoadJob: public qtuser_core::Job
	{
	public:
		CADLoadJob(QObject* parent = nullptr);
		virtual ~CADLoadJob();

		void setFileName(const QString& fileName);
		void setModelNDataProcessor(cxkernel::ModelNDataProcessor* processor);
	protected:
		QString name();
		QString description();
		void failed();                        // invoke from main thread
		void successed(qtuser_core::Progressor* progressor);                     // invoke from main thread
		void work(qtuser_core::Progressor* progressor);    // invoke from worker thread

	protected:
		QString m_fileName;
		cxkernel::ModelNDataProcessor* m_processor;
		cxkernel::ModelNDataPtr m_data;
	};
}
#endif // _CREATIVE_KERNEL_MESHLOADJOB_1590984808257_H
