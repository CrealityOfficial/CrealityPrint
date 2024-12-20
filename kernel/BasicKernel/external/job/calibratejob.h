#ifndef _INSCRIBEDJOB_1681993975189_H
#define _INSCRIBEDJOB_1681993975189_H
#include "qtusercore/module/job.h"
#include "cxkernel/data/modelndata.h"

#include "crslice/crscene.h"
#include "crslice/crslice.h"
#include "crslice2/crscene.h"
#include "crslice2/crslice.h"

#include "entity/gcodeviewentity.h"
#include "cxgcode/gcodeloadjob.h"
#include "cxgcode/sliceattain.h"
#include "cxkernel/interface/jobsinterface.h"
#include "slice/calibrate.h"

namespace creative_kernel
{
	class Calibratejob : public qtuser_core::Job
	{
	public:
		Calibratejob(creative_kernel::SliceFlow* _sliceflow, QObject* parent = nullptr);
		virtual ~Calibratejob();

		void setFileName(const QString& fileName);
		void setSceneCreator(crslice::SceneCreator* creator);
		void setSceneCreatorOrca(crslice2::SceneCreator* creator);
	protected:
		QString name();
		QString description();
		void failed();                        // invoke from main thread
		void successed(qtuser_core::Progressor* progressor);                     // invoke from main thread
		void work(qtuser_core::Progressor* progressor);    // invoke from worker thread
		void setColor(crslice2::SettingsPtr _settings);
	protected:
		QString m_fileName;
		crslice::SceneCreator* m_creator;
		crslice2::SceneCreator* m_creatorOrca;
		QScopedPointer<cxgcode::SliceAttain> m_attain;
		//FDM52Flow* m_flow;
		creative_kernel::SliceFlow* m_sliceflow;

		QString m_gcodeFile;
	};
}

#endif // _INSCRIBEDJOB_1681993975189_H
