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
				emit sliceSuccess(m_remainAttain.take());
			}
			else
				emit sliceFailed();
		}
		else
		{
			emit sliceSuccess(m_sliceAttain);
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

			QScopedPointer<AnsycSlicer> slicer(new DLLAnsycSlicer52());
			SliceResultPointer result(slicer->doSlice(input, tracer));
			if (result)
			{
				result->setSliceName(generateSceneName().toLocal8Bit().data());
				//result->G = convert(input.G);
				//result->ES = convert(input.Es);
				result->inputBox = input.sceneBox();

				m_sliceAttain = new SliceAttain(result, SliceAttainType::sat_slice);
				m_sliceAttain->build(&tracer);

				qDebug() << QString("Slice : SliceAttain build over . [%1]").arg(currentProcessMemory());
			}
		}
	}
}
