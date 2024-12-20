#ifndef _CREATIVE_KERNEL_MESHLOADJOB_1590984808257_H
#define _CREATIVE_KERNEL_MESHLOADJOB_1590984808257_H
#include "qtusercore/module/job.h"
#include "cxkernel/kernel/data.h"
#include "cxkernel/data/modelndata.h"
#include "cxkernel/utils/meshjob.h"

#ifndef CXKERNEL_DISABLE_3MF
#include "common_3mf.h"
#endif

namespace cxkernel
{
	class MeshLoadJob: public MeshJob
	{
	public:
		MeshLoadJob(QObject* parent = nullptr);
		virtual ~MeshLoadJob();

		void setFileName(const QString& fileName);

		void setModelNDataProcessor(ModelNDataProcessor* processor);

	protected:
		QString name();
		QString description();
		void failed();                        // invoke from main thread
		void successed(qtuser_core::Progressor* progressor);                     // invoke from main thread
		void work(qtuser_core::Progressor* progressor);    // invoke from worker thread
	protected:
		QString m_fileName;
		TriMeshPtr m_mesh;
#ifndef CXKERNEL_DISABLE_3MF
		common_3mf::Scene3MF m_scene;
#endif
		ModelNDataProcessor* m_processor;
	};
}
#endif // _CREATIVE_KERNEL_MESHLOADJOB_1590984808257_H
