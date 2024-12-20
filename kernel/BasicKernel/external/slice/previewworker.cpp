#include "previewworker.h"
#include <QtCore/QFileInfo>
#include <QtCore/QDebug>

#include "cxkernel/interface/jobsinterface.h"
#include "interface/commandinterface.h"
#include "interface/appsettinginterface.h"
#include "slice/sliceflow.h"
#include "slice/gcodefilepreview.h"
#include "interface/spaceinterface.h"
#include "kernel/kernel.h"

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
		auto isTextFile = [](const QString& fileName) {

			QFile file(fileName);
			if (!file.open(QIODevice::ReadOnly))
			{
				return false;
			}

			QByteArray fileData = file.read(1024);
			file.close();

			for (char c : fileData)
			{
				if (c == '\0')
				{
					return false;
				}
			}

			return true;
		};

		bool check = false;
		if (suffix == "gcode" && isTextFile(fileName))
		{
			check = true;
		}
		if (check)
		{
			base = new GcodeFilePreview();

			QString imgSavePath = QString("%1/imgPreview.%2").arg(SLICE_PATH).arg("png");
			QFile(imgSavePath).remove();
		}

		if (base)
		{
			m_receiver->setStartAction(SliceFlow::PreviewGCodeFile);
			if (getKernel()->currentPhase() != 1)
				setKernelPhase(KernelPhaseType::kpt_preview);
			else
				m_receiver->prepareGcodePreview();

			connect(base, SIGNAL(sliceSuccess(SliceAttain*)), m_receiver, SLOT(onParseGcodeSuccuess(SliceAttain*)));
			connect(base, SIGNAL(sliceFailed()), m_receiver, SLOT(onSliceFailed()));
			base->setFileName(fileName);
            dirtyModelSpace();
			cxkernel::executeJob(QSharedPointer<qtuser_core::Job>(base));
		}
	}
}
