#ifndef _CXKERNEL_MODEL_FROM_MESH_JOB_1590984808257_H
#define _CXKERNEL_MODEL_FROM_MESH_JOB_1590984808257_H
#include "qtusercore/module/job.h"
#include "cxkernel/data/modelndata.h"

namespace cxkernel
{
	class ModelFromMeshJob: public qtuser_core::Job
	{
	public:
		ModelFromMeshJob(ModelNDataProcessor* processor, QObject* parent = nullptr);
		virtual ~ModelFromMeshJob();

		void setInput(ModelCreateInput input);
		void setParam(const ModelNDataCreateParam& param);
	protected:
		QString name();
		QString description();
		void failed();                        // invoke from main thread
		void successed(qtuser_core::Progressor* progressor);                     // invoke from main thread
		void work(qtuser_core::Progressor* progressor);    // invoke from worker thread

	protected:
		ModelCreateInput m_input;
		ModelNDataPtr m_data;
		ModelNDataProcessor* m_processor;

		ModelNDataCreateParam m_param;
	};
}
#endif // _CXKERNEL_MODEL_FROM_MESH_JOB_1590984808257_H
