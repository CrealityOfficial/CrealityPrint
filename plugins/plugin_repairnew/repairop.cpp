#include "repairop.h"

#include <QtCore/QDebug>  

#include "interface/visualsceneinterface.h"
#include "interface/eventinterface.h"
#include "cxkernel/interface/jobsinterface.h"
#include "interface/reuseableinterface.h"
#include "interface/selectorinterface.h"

#include "data/modeln.h"
#include "repairjob.h"
#include <QList>
#include "kernel/kernel.h"
using namespace creative_kernel;
using namespace qtuser_3d;

RepairOp::RepairOp(QObject* parent)
	:SceneOperateMode(parent)
{
}

RepairOp::~RepairOp()
{
}

void RepairOp::repairModel(QList<creative_kernel::ModelN*> selections)
{
#ifdef Q_OS_WINDOWS
	RepairJob* job = new RepairJob();
	for (size_t i = 0; i < selections.size(); i++)
	{
		ModelN* m = selections.at(i);
		job->setModel(m);
	}
	cxkernel::executeJob(qtuser_core::JobPtr(job));
	disconnect(getKernel()->jobExecutor(), SIGNAL(jobsEnd()), this, SIGNAL(repairSuccess()));
	connect(getKernel()->jobExecutor(), SIGNAL(jobsEnd()), this, SIGNAL(repairSuccess()));
#endif
}
void RepairOp::onAttach()
{
	//addLeftMouseEventHandler(this);
	//addHoverEventHandler(this);
}

void RepairOp::onDettach()
{
	//removeLeftMouseEventHandler(this);
	//removeHoverEventHandler(this);

}

void RepairOp::onLeftMouseButtonClick(QMouseEvent* event)
{
#ifdef Q_OS_WINDOWS
	ModelN* model = checkPickModel(event->pos());
	if(model)
	{
		RepairJob* job = new RepairJob();
		job->setModel(model);
		cxkernel::executeJob(qtuser_core::JobPtr(job));
	}
#endif
}

void RepairOp::onHoverMove(QHoverEvent* event)
{
	//
}

/*
void RepairOp::judgeModel(QList<creative_kernel::ModelN*> selections)
{
	JudgeModelJob* job = new JudgeModelJob();
	for (size_t i = 0; i < selections.size(); i++)
	{
		ModelN* m = selections.at(i);
		job->setModel(m);
	}
	executeJob(JobPtr(job));
	disconnect(getKernel()->jobExecutor(), SIGNAL(jobsEnd()), this, SIGNAL(judgeSuccess()));
	connect(getKernel()->jobExecutor(), SIGNAL(jobsEnd()), this, SIGNAL(judgeSuccess()));
}
*/