#ifndef _CREATIVE_KERNEL_SPLIT_MODEL_JOB_H
#define _CREATIVE_KERNEL_SPLIT_MODEL_JOB_H
#include "basickernelexport.h"
#include "qtusercore/module/job.h"
#include "data/modeln.h"

namespace creative_kernel
{
	class SplitModelJob : public qtuser_core::Job
	{
	public:
		SplitModelJob(QObject* parent = nullptr);
		virtual ~SplitModelJob();

		void EnableSplit(bool bSplit);
		void setModels(const QList<ModelN*> model);
	protected:
		QString name();
		QString description();
		void failed();                        // invoke from main thread
		void successed(qtuser_core::Progressor* progressor);                     // invoke from main thread
		void work(qtuser_core::Progressor* progressor);    // invoke from worker thread

	public:
		void SplitModel(qtuser_core::Progressor* progressor);
		void MergeModel(qtuser_core::Progressor* progressor);

	private:
		bool m_bSplit;
		QList<ModelN*> m_models;
		std::vector<cxkernel::ModelNDataPtr> m_datas;
	};
}

#endif // _CREATIVE_KERNEL_MESHLOADJOB_1590984808257_H
