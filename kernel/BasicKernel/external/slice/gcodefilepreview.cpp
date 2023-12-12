#include "gcodefilepreview.h"
#include "slice/sliceattain.h"
#include "slice/ansycworker.h"

#include "qtusercore/module/systemutil.h"
#include "qtusercore/module/progressortracer.h"

#include <QtCore/QFileInfo>

namespace creative_kernel
{
	GcodeFilePreview::GcodeFilePreview(QObject* parent)
		:PreviewBase(parent)
	{
	}

	GcodeFilePreview::~GcodeFilePreview()
	{
	}

	void GcodeFilePreview::work(qtuser_core::Progressor* progressor)
	{
		SliceAttainTracer tracer(progressor);

		float stepProgress = 0.8f;

		tracer.resetProgressScope(0.0f, stepProgress);

		SliceResultPointer result(new gcode::SliceResult());
		result->load(m_fileName.toLocal8Bit().data(), &tracer);

		qDebug() << QString("Slice : GcodeFilePreview load over . [%1]").arg(currentProcessMemory());
		if (!result->layerCode().empty())
		{
			QFileInfo fileInfo(m_fileName);
			result->setSliceName(fileInfo.baseName().toLocal8Bit().data());

			tracer.resetProgressScope(stepProgress, 1.0f);
			m_attain = new SliceAttain(result, SliceAttainType::sat_file);
			m_attain->build(&tracer);
			qDebug() << QString("Slice : SliceAttain build over . [%1]").arg(currentProcessMemory());
		}
		else
		{
			progressor->failed(QString("GcodeFilePreview::work load gcode file %1 failed.").arg(m_fileName));
		}
		progressor->progress(1.0);
	}
}