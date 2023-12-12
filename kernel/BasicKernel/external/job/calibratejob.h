#ifndef _INSCRIBEDJOB_1681993975189_H
#define _INSCRIBEDJOB_1681993975189_H
#include "qtusercore/module/job.h"
#include "cxkernel/data/modelndata.h"

#include "crslice/crscene.h"
#include "crslice/crslice.h"

#include "entity/gcodeviewentity.h"
#include "cxgcode/gcodeloadjob.h"
#include "cxgcode/sliceattain.h"
#include "cxkernel/interface/jobsinterface.h"
#include "slice/calibrate.h"

//class FDM52Flow;
class Calibratejob : public qtuser_core::Job
{
public:
	Calibratejob(creative_kernel::SliceFlow* _sliceflow, QObject* parent = nullptr);
	virtual ~Calibratejob();

	void setFileName(const QString& fileName);
	void setSceneCreator(crslice::SceneCreator* creator);
protected:
	QString name();
	QString description();
	void failed();                        // invoke from main thread
	void successed(qtuser_core::Progressor* progressor);                     // invoke from main thread
	void work(qtuser_core::Progressor* progressor);    // invoke from worker thread
protected:
	QString m_fileName;
	crslice::SceneCreator* m_creator;
	QScopedPointer<cxgcode::SliceAttain> m_attain;
	//FDM52Flow* m_flow;
	creative_kernel::SliceFlow* m_sliceflow;

	QString m_gcodeFile;
};

QString generateTempGCodeFileName();

#endif // _INSCRIBEDJOB_1681993975189_H
