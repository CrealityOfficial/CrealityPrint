#ifndef _CREATIVE_KERNEL_SPLIT_MODEL_2_PARTS_JOB_H
#define _CREATIVE_KERNEL_SPLIT_MODEL_2_PARTS_JOB_H
#include "basickernelexport.h"
#include "qtusercore/module/job.h"
#include "data/modeln.h"
#include "data/rawdata.h"

namespace creative_kernel
{
	class SplitModel2PartsJob : public qtuser_core::Job
	{
	public:
		SplitModel2PartsJob(QObject* parent = nullptr);
		virtual ~SplitModel2PartsJob();

		void setModel(ModelN* model);
		void setSplit2ObjectFlag(bool bFlag);
	protected:
		QString name();
		QString description();
		void failed();                        // invoke from main thread
		void successed(qtuser_core::Progressor* progressor);                     // invoke from main thread
		void work(qtuser_core::Progressor* progressor);    // invoke from worker thread

		void SplitModel(qtuser_core::Progressor* progressor);
		void createParts();
		void createObjects();

	private:
		ModelN* m_model;
		QList<ModelDataPtr> m_datas;
		QList<QVector3D> m_componentOffsets;

		QList<QVector3D> m_split2ObjectPoses;
		bool m_split2ObjectFlag;
	};
}

#endif // _CREATIVE_KERNEL_SPLIT_MODEL_2_PARTS_JOB_H
