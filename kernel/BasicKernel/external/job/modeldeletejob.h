#ifndef MODEL_DELETE_INSCRIBEDJOB_1681993975189_H
#define MODEL_DELETE_INSCRIBEDJOB_1681993975189_H
#include "qtusercore/module/job.h"
#include "data/modeln.h"

namespace creative_kernel
{
	class ModelDeleteJob : public qtuser_core::Job
	{
	public:
		ModelDeleteJob(QObject* parent = nullptr);
		virtual ~ModelDeleteJob();

		void setModel(ModelN* model);
		void setModelGroup(ModelGroup* modelGroup);
	protected:
		QString name();
		QString description();
		void failed();                        // invoke from main thread
		void successed(qtuser_core::Progressor* progressor);                     // invoke from main thread
		void work(qtuser_core::Progressor* progressor);    // invoke from worker thread
	protected:
		ModelN* m_model;
		ModelGroup* m_modelGroup;
	};
}

#endif // _INSCRIBEDJOB_1681993975189_H
