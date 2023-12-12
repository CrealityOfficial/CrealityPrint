#include "ansycworker.h"
#include "qtusercore/module/systemutil.h"
#include "qtusercore/module/progressortracer.h"

#include "sliceattain.h"
#include "dllansycslicer52.h"

#include <QtCore/QTimeLine>
#include <QtCore/QTime>
#include<QStandardPaths>

#include "interface/spaceinterface.h"
#include "interface/appsettinginterface.h"
#include "interface/commandinterface.h"
#include "interface/machineinterface.h"

namespace creative_kernel
{
	SliceAttainTracer::SliceAttainTracer(qtuser_core::Progressor* progressor)
			: FormatTracer(progressor)
	{
	}

	int SliceAttainTracer::indexCount() {
		return 2;
	}

	QString SliceAttainTracer::indexStr(int index, va_list vars)
	{
		int layer = va_arg(vars, int);
		QString message = tr("");
		if (index == 0)
			message = tr("Start build render %1 layer").arg(layer);

		return message;
	}


	FormatSlice::FormatSlice(qtuser_core::Progressor* progressor)
	:FormatTracer(progressor)
	{
	}


	int FormatSlice::indexCount()
	{
		return 12;
	}

	QString FormatSlice::indexStr(int index, va_list vars)
	{
		int layer = va_arg(vars, int);
		QString message = tr("");
		if (index == 0)
			message = tr("Start slicing");
		if (index == 1)
			message = tr("Model layering in progress");
		if (index == 2)
			message = tr("Generating polygons");
		if (index == 3)
			message = tr("Generating walls");
		if (index == 4)
			message = tr("Generating skin and infill");
		if (index == 5)
			message = tr("Checking support necessity");
		if (index == 6)
			message = tr("Generating support area");
		if (index == 7)
			message = tr("Optimizing ZSeam");
		if (index == 8)
			message = tr("Generating gcode %1 layer").arg(layer);
		if (index == 9)
			message = tr("Slicing finished");
		if (index == 11)
			message = tr("need_support_structure");
		return message;
	}


	SettingsPointer convert(SettingsPointer setting)
	{
		SettingsPointer result(new us::USettings());
		const QHash<QString, us::USetting*>& settings = setting->settings();
		for (QHash<QString, us::USetting*>::ConstIterator it = settings.begin();
			it != settings.end(); ++it)
		{
			result->add(it.key(), it.value()->str(), true);
		}

		return result;
	}

	QList<SettingsPointer> convert(const QList<SettingsPointer>& settings)
	{
		QList<SettingsPointer> result;
		for (const SettingsPointer& setting : settings)
			result.append(convert(setting));
		return result;
	}

	AnsycWorker::AnsycWorker(QObject* parent)
		:Job(parent)
		, m_sliceAttain(nullptr)
	{
	}

	AnsycWorker::~AnsycWorker()
	{
	}

	SliceAttain* AnsycWorker::sliceAttain()
	{
		return m_sliceAttain;
	}

	void AnsycWorker::setRemainAttain(SliceAttain* attain)
	{
		m_remainAttain.reset(attain);
	}

	QString AnsycWorker::name()
	{
		return "Slice AnsycWorker.";
	}

	QString AnsycWorker::description()
	{
		return "Slice AnsycWorker.";
	}

	void AnsycWorker::failed()
	{
		emit sliceFailed();
	}

	void AnsycWorker::successed(qtuser_core::Progressor* progressor)
	{
		if (!m_sliceAttain)
		{
			if (m_remainAttain.data())
			{
				emit sliceSuccess(m_remainAttain.take(), true);
			}
			else
				emit sliceFailed();
		}
		else
		{
			emit sliceSuccess(m_sliceAttain, false);
			m_sliceAttain = nullptr;
		}

		clearModelSpaceDrity();
		clearSettingsDirty();
	}

	bool AnsycWorker::needReSlice()
	{
		if (modelns().size() > 0 && (modelSpaceDirty() || settingsDirty()))
			return true;

		if (m_remainAttain.isNull())
			return true;

		return false;
	}

#if 1
	void AnsycWorker::work(qtuser_core::Progressor* progressor)
	{
		if (!needReSlice())
			return;

		//if (modelns().size() == 0)
		//{
		//	progressor->failed("nothing to slice.");
		//	return;
		//}

		SliceInput input;
		produceSliceInput(input);

		if (!input.hasModel())
		{
			progressor->failed("nothing to slice.");
			return;
		}

		if (input.canSlice())
		{
			FormatSlice tracer(progressor);

			QScopedPointer<AnsycSlicer> slicer(new DLLAnsycSlicer52());
			SliceResultPointer result(slicer->doSlice(input, tracer));
			if (result)
			{
				result->setSliceName(generateSceneName().toLocal8Bit().data());
				//result->G = convert(input.G);
				//result->ES = convert(input.Es);
				result->inputBox = input.sceneBox();

				SliceAttainTracer attainTracer(progressor);
				m_sliceAttain = new SliceAttain(result, SliceAttainType::sat_slice);
				m_sliceAttain->build(&attainTracer);

				emit sliceBeforeSuccess(m_sliceAttain);

				qDebug() << QString("Slice : SliceAttain build over . [%1]").arg(currentProcessMemory());
			}
		}
	}
#else // 

	void AnsycWorker::work(qtuser_core::Progressor* progressor)
	{
		if (!needReSlice())
			return;

		if (modelns().size() == 0)
		{
			progressor->failed("nothing to slice.");
			return;
		}

		SliceInput input;
		produceSliceInput(input);

		if (input.canSlice())
		{
			qtuser_core::ProgressorMessageTracer tracer{ [this](const char* message) {
				Q_EMIT sliceMessage(QString{ message });
			}, progressor };


			SliceResultPointer result(new gcode::SliceResult());
			result->setSliceName(generateSceneName().toLocal8Bit().data());
			result->inputBox = input.sceneBox();
			m_sliceAttain = new SliceAttain(result, SliceAttainType::sat_slice);

			connect(m_sliceAttain, SIGNAL(layerChanged(int)), this, SIGNAL(gcodeLayerChanged(int)));

			QScopedPointer<AnsycSlicer> slicer(new DLLAnsycSlicer52());
		    slicer->doSlice(input, tracer, m_sliceAttain);
			if (result)
			{
				//m_sliceAttain = new SliceAttain(result, SliceAttainType::sat_slice);
				//m_sliceAttain->build(&tracer);		

				//result->G = convert(input.G);
				//result->ES = convert(input.Es);
				

				//load temp gcode
				QString fileName = QString("%1/temp.gcode").arg(SLICE_PATH);
				result->load(fileName.toLocal8Bit().data());

				m_sliceAttain->build_preview(&tracer);

				emit sliceBeforeSuccess(m_sliceAttain);

				tracer.resetProgressScope(1.0f, 1.0f);

				qDebug() << QString("Slice : SliceAttain build over . [%1]").arg(currentProcessMemory());
			}
		}
	}
#endif //
}
