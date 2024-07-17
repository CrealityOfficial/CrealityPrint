#include "mirrorundocommand.h"
#include "internal/data/_modelinterface.h"
#include "external/job/mirrorjob.h"
#include "cxkernel/interface/jobsinterface.h"
#include "interface/spaceinterface.h"

namespace creative_kernel
{
	MirrorUndoCommand::MirrorUndoCommand(QObject* parent)
		:QObject(parent)
	{
	}
	
	MirrorUndoCommand::~MirrorUndoCommand()
	{
	}

	void MirrorUndoCommand::setModels(const QList<ModelN*>& models)
	{
		for (ModelN* model : models)
		{
			m_serialNames << model->getSerialName();
		}
	}

	void MirrorUndoCommand::setMirrorMode(int mode)
	{
		m_mode = mode;
	}

	void MirrorUndoCommand::undo()
	{
		call(false);
	}

	void MirrorUndoCommand::redo()
	{
		call(true);
	}

	void MirrorUndoCommand::call(bool _redo)
	{
		QList<ModelN*> models;
		for (QString name : m_serialNames)
		{
			models << findModelBySerialName(name);
		}

		MirrorJob* job = new MirrorJob(models, m_mode);
		cxkernel::executeJob(qtuser_core::JobPtr(job));
	}
}