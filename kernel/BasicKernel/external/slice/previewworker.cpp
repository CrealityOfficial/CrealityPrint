#include "previewworker.h"
#include <QtCore/QFileInfo>
#include <QtCore/QDebug>

#include "cxkernel/interface/jobsinterface.h"
#include "interface/commandinterface.h"
#include "slice/sliceflow.h"
#include "slice/gcodefilepreview.h"
#include "interface/spaceinterface.h"

namespace creative_kernel
{
	PreviewWorker::PreviewWorker(QObject* parent)
		: QObject(parent)
		, m_receiver(nullptr)
	{
		m_receiver = qobject_cast<SliceFlow*>(parent);
	}

	PreviewWorker::~PreviewWorker()
	{
	}

	QString PreviewWorker::filter()
	{
		QString _filter = QString("GCode File (*.gcode)");
		return _filter;
	}

	void PreviewWorker::handle(const QString& fileName)
	{
		QFileInfo info(fileName);
		QString suffix = info.suffix();
		suffix = suffix.toLower();

		PreviewBase* base = nullptr;

		bool check = false;
		if (suffix == "gcode")
		{
			check = true;
		}
		if (check)
		{
			base = new GcodeFilePreview();
		}

		if (base)
		{
			setKernelPhase(KernelPhaseType::kpt_preview);
			connect(base, SIGNAL(sliceSuccess(SliceAttain*)), m_receiver, SLOT(onSliceSuccess(SliceAttain*)));
			connect(base, SIGNAL(sliceFailed()), m_receiver, SLOT(onSliceFailed()));
			base->setFileName(fileName);
            dirtyModelSpace();
			cxkernel::executeJob(QSharedPointer<qtuser_core::Job>(base));
		}
	}
}
