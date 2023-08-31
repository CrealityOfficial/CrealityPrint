#include "ansycworkerchengfei.h"
#include "qtusercore/module/systemutil.h"
#include "qtusercore/module/progressortracer.h"

#include "slice/sliceattain.h"
#include "dllansycslicer52chengfei.h"

#include <QtCore/QTimeLine>
#include <QtCore/QTime>
#include<QStandardPaths>

#include "interface/spaceinterface.h"
#include "interface/appsettinginterface.h"
#include "interface/commandinterface.h"
#include "interface/machineinterface.h"

namespace creative_kernel
{
	SettingsPointer convertcf(SettingsPointer setting)
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

	QList<SettingsPointer> convertcf(const QList<SettingsPointer>& settings)
	{
		QList<SettingsPointer> result;
		for (const SettingsPointer& setting : settings)
			result.append(convertcf(setting));
		return result;
	}

	AnsycWorkerCF::AnsycWorkerCF(const chengfeiSplit& chengfeiSplit,QObject* parent)
		:Job(parent)
		, m_sliceAttain(nullptr)
		,m_chengfeiSplit(chengfeiSplit)
	{
	}

	AnsycWorkerCF::~AnsycWorkerCF()
	{
	}

	void AnsycWorkerCF::setRemainAttain(SliceAttain* attain)
	{
		m_remainAttain.reset(attain);
	}

	QString AnsycWorkerCF::name()
	{
		return "Slice AnsycWorker.";
	}

	QString AnsycWorkerCF::description()
	{
		return "Slice AnsycWorker.";
	}

	void AnsycWorkerCF::failed()
	{
		emit sliceFailed();
	}

	void AnsycWorkerCF::successed(qtuser_core::Progressor* progressor)
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

	bool AnsycWorkerCF::needReSlice()
	{
		if (modelns().size() > 0 && (modelSpaceDirty() || settingsDirty()))
			return true;

		if (m_remainAttain.isNull())
			return true;

		return false;
	}

	void AnsycWorkerCF::work(qtuser_core::Progressor* progressor)
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
			QScopedPointer<DLLAnsycSlicer52CF> slicer(new DLLAnsycSlicer52CF(m_chengfeiSplit));
			qtuser_core::ProgressorTracer tracer(progressor);
			SliceResultPointer result(slicer->doSlice(input, tracer));
			if (result)
			{
				result->setSliceName(generateSceneName().toLocal8Bit().data());
				//result->G = convertcf(input.G);
				//result->ES = convertcf(input.Es);
				result->inputBox = input.sceneBox();

				m_sliceAttain = new SliceAttain(result, SliceAttainType::sat_slice);
				m_sliceAttain->build(&tracer);

				qDebug() << QString("Slice : SliceAttain build over . [%1]").arg(currentProcessMemory());
			}
		}
	}
}
