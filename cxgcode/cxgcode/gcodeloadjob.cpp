#include "gcodeloadjob.h"

#include "crslice/gcode/sliceresult.h"
#include "cxgcode/sliceattain.h"

#include "qtusercore/module/systemutil.h"
#include "qtusercore/module/progressortracer.h"

#include <QtCore/QFileInfo>
#include <QtCore/QDebug>

namespace cxgcode
{
	GCodeLoadJob::GCodeLoadJob(QObject* parent)
		: Job(parent)
		, m_attain(nullptr)
	{
	}

	GCodeLoadJob::~GCodeLoadJob()
	{

	}

	void GCodeLoadJob::setFileName(const QString& fileName)
	{
		m_fileName = fileName;
	}

	void GCodeLoadJob::setSlicePath(const QString& slicePath)
	{
		m_slicePath = slicePath;
	}

	void GCodeLoadJob::setTempPath(const QString& tempPath)
	{
		m_tempPath = tempPath;
	}

	QString GCodeLoadJob::name()
	{
		return "GCodeLoadJob";
	}

	QString GCodeLoadJob::description()
	{
		return "GCodeLoadJob";
	}

	void GCodeLoadJob::failed()
	{
		qDebug() << "GCodeLoadJob failed.";
		emit sliceFailed();
	}

	void GCodeLoadJob::successed(qtuser_core::Progressor* progressor)
	{
		qDebug() << "GCodeLoadJob success";
		SliceAttain* attain = take();

		assert(attain);
		if (attain)
			emit sliceSuccess(attain);
	}

	void GCodeLoadJob::work(qtuser_core::Progressor* progressor)
	{
		qtuser_core::ProgressorTracer tracer(progressor);
		float stepProgress = 0.8f;

		tracer.resetProgressScope(0.0f, stepProgress);

		SliceResultPointer result(new gcode::SliceResult());
		result->load(m_fileName.toStdString(), &tracer);

		qDebug() << QString("Slice : GcodeFilePreview load over . [%1]").arg(currentProcessMemory());
		if (!result->layerCode().empty())
		{
			QFileInfo fileInfo(m_fileName);
			result->setSliceName(fileInfo.baseName().toStdString());

			tracer.resetProgressScope(stepProgress, 1.0f);
			m_attain = new SliceAttain(m_slicePath, m_tempPath, result, SliceAttainType::sat_file);
			m_attain->build(&tracer);
			qDebug() << QString("Slice : SliceAttain build over . [%1]").arg(currentProcessMemory());
		}
		else
		{
			progressor->failed(QString("GcodeFilePreview::work load gcode file %1 failed.").arg(m_fileName));
		}
		progressor->progress(1.0);
	}

	SliceAttain* GCodeLoadJob::take()
	{
		SliceAttain* attain = m_attain;
		m_attain = nullptr;
		return attain;
	}
}