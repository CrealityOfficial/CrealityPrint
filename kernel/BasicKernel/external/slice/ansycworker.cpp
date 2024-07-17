#include "ansycworker.h"
#include "qtusercore/module/systemutil.h"
#include "qtusercore/module/progressortracer.h"

#include "sliceattain.h"
#include "dllansycslicer52.h"
#include "dllansycslicerorca.h"

#include <QtCore/QTimeLine>
#include <QtCore/QTime>
#include<QStandardPaths>
#include <QCoreApplication>

#include "interface/spaceinterface.h"
#include "interface/appsettinginterface.h"
#include "interface/commandinterface.h"
#include "interface/machineinterface.h"
#include "interface/printerinterface.h"
#include "interface/messageinterface.h"
#include "internal/multi_printer/printer.h"
#include "message/sliceenginefailmessage.h"

#include "data/modeln.h"
#include "crslice2/crscene.h"
#include "internal/parameter/parametermanager.h"
#include "internal/parameter/printmachine.h"
#include "internal/parameter/printprofile.h"
#include "kernel/kernel.h"
#include "cxgcode/gcodehelper.h"
#include "cxkernel/utils/utils.h"
#include <external/interface/uiinterface.h>

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

	Crslice2Tracer::Crslice2Tracer(qtuser_core::Progressor* progressor, QObject* parent)
		:ProgressorTracer(progressor, parent)
	{
	}

	Crslice2Tracer::~Crslice2Tracer()
	{
	}

	void Crslice2Tracer::message(const char* msg)
	{
		if (m_progressor)
		{
			QString qstrMsg = msg;
			if (qstrMsg.startsWith("${UnitTest}"))
			{
				QString realMsg = qstrMsg.mid(QString("${UnitTest}").length());
				requestQmlTipDialog(realMsg);
			}
			else if (qstrMsg.startsWith("@@"))
			{
				QString realMsg = qstrMsg.mid(2);
				MessageObject* message = new Crslice2InfoMessage(realMsg);
				invokeBriefMessage(message);
				return;
			}
			else if (qstrMsg.contains("layer"))
			{
				QStringList list = qstrMsg.split("layer");
				if (list.size() != 2)
				{
					QString lan = QCoreApplication::translate("ParameterComponent", msg);
					m_progressor->message(lan);
				}
				else
				{
					QString msg2 = list[0] + "layer %1";
					QByteArray ba = msg2.toLatin1();
					int layer = list[1].toInt();
					QString lan = QCoreApplication::translate("ParameterComponent", ba.data()).arg(layer);
					m_progressor->message(lan);
				}
			}
			else
			{
				QString lan = QCoreApplication::translate("ParameterComponent", msg);
				m_progressor->message(lan);
			}
		}
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
		m_remainAttain = attain;
	}
	
	void AnsycWorker::setSlicePrinter(Printer* printer)
	{
		m_printer = printer;
	}

	Printer* AnsycWorker::slicePrinter()
	{
		return m_printer;
	}

	void AnsycWorker::applyAdditionalSliceParameter()
	{
		/* default_filament_colour 使用到的耗材列表 */
		QSet<int> usedColorIndexes;
		QList<us::USettings*> settingsList;
		for (ModelN* model : m_slicedModels)
		{
			usedColorIndexes += model->data()->colorIndexs;
			settingsList << model->setting();
		}

		auto parameterManager = getKernel()->parameterManager();
		auto settingPtr = parameterManager->currentMachine()->createCurrentGlobalSettings();
		settingsList << settingPtr.get();

		for (auto settings : settingsList)
		{
			QString support = settings->value("support_filament", "0");
			if (support != "0")
			{
				int index = support.toInt() - 1;
				if (index >= 0)
					usedColorIndexes.insert(index);
			}

			QString supportInterface = settings->value("support_interface_filament", "0");
			if (supportInterface != "0")
			{
				int index = supportInterface.toInt() - 1;
				if (index >= 0)
					usedColorIndexes.insert(index);
			}

			QString filamentColour;
			std::vector<trimesh::vec> colors = currentColors();
			for (int i = 0, count = colors.size(); i < count; ++i)
			{
				QString colorStr;
				if (usedColorIndexes.contains(i))
				{
					trimesh::vec color = colors[i];
					colorStr = QString("#%1%2%3").arg((int)(color[0] * 255), 2, 16, QChar('0')).
						arg((int)(color[1] * 255), 2, 16, QChar('0')).
						arg((int)(color[2] * 255), 2, 16, QChar('0'));
				}
				filamentColour += (colorStr + ";");
			}
			if (filamentColour.endsWith(";"))
				filamentColour = filamentColour.remove(filamentColour.length() - 1,1);

			settings->add("default_filament_colour", filamentColour, true);
		}
	}

	void AnsycWorker::waitForCompleted()
	{
		while (m_slicing)
		{
			QThread::msleep(20);
			QCoreApplication::processEvents();
		}
	}

	void AnsycWorker::cancel()
	{

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
		emit sliceFailed(m_failExceptionDesc);
		if (m_sliceAttain)
			delete m_sliceAttain;
		m_slicing = false;
	}

	void AnsycWorker::successed(qtuser_core::Progressor* progressor)
	{
		if (!m_sliceAttain)
		{
			SliceAttain* attain = dynamic_cast<SliceAttain*>(m_printer->attain());
			if (attain)
				emit sliceSuccess(attain, m_printer);
			else
				emit sliceFailed(m_failExceptionDesc);
		}
		else
		{
			QVector3D pathSize = m_sliceAttain->getGCodePathBoundingBox().size();
			QVector3D printerSize = m_printer->box().size();
			
			//gcode轨迹超出平台可打印区域
			//if (pathSize.x() > printerSize.x() || pathSize.y() > printerSize.y())
			//{
			//	float x = pathSize.x() > printerSize.x() ? printerSize.x() - pathSize.x() : 0.0f;
			//	float y = pathSize.y() > printerSize.y() ? printerSize.y() - pathSize.y() : 0.0f;
			//
			//	QVector3D offset = QVector3D(x, y, 0.0f);
			//	m_printer->moveWipeTower(offset);
			//
			//	//TODO:slice Failed
			//	return;
			//}



			for (auto m : m_slicedModels)
			{
				m->recordSetting();
				m->resetDirty();
			}

			clearSettingsDirty();

			// SliceAttain* sliceCache = dynamic_cast<SliceAttain*>(m_printer->sliceCache());
			// if (sliceCache)
			// {
			// 	if (sliceCache->layers() != m_sliceAttain->layers())
			// 	{
			// 		m_printer->clearLayersConfig();
			// 	}
			// }
			m_printer->resetDirty();
			m_printer->setAttain(m_sliceAttain);
			m_printer->setIsSliced(true);
			emit sliceSuccess(m_sliceAttain, m_printer);
			m_sliceAttain = nullptr;
		}
		

		m_slicing = false;
	}

	bool AnsycWorker::needReSlice()
	{
		if (m_printer->isDirty())
			return true;

		bool modelDirty = false;
		for (auto m : m_slicedModels)
		{
			if (m->isDirty())
			 	modelDirty = true;
		}

		if (modelns().size() > 0 && modelDirty)
			return true;

		return false;
	}

	void AnsycWorker::prepareSliceData()
	{
		/* init model list */
		m_slicedModels = m_printer->modelsInside();

		/* init picture */
		QImage image = m_printer->picture();
		auto parameterManager = getKernel()->parameterManager();
		QString thumbnails = parameterManager->currentMachine()->settingsValue("thumbnails");
		//QString thumbnails = "96x96,300x300";
		QList<QPair<int, int>> sizes;
		QStringList sizeStrs = thumbnails.split(",");
		for (QString str : sizeStrs)
		{
			QStringList splits = str.split("x");
			sizes << QPair<int, int>(splits[0].toInt(), splits[1].toInt());
		}

		m_thumbnailDatas.clear();
		for (auto size : sizes)
		{
		    QImage tempImg = image.scaled(QSize(size.first, size.second), Qt::KeepAspectRatio, Qt::SmoothTransformation);

			auto ptr = tempImg.bits();
			crslice2::ThumbnailData data;
			int start = cxsw::getLineStart(&tempImg);
			int end = cxsw::getLineEnd(&tempImg);
			data.pos_s = start;
			data.pos_e = end;
			
			for (int y = size.second - 1; y >= 0; --y)
			{
				int base = y * size.first;
				for (int x = 0; x < size.first; ++x)
				{
					int index = (base + x) * 4;
					data.pixels.push_back(ptr[index + 2]);
					data.pixels.push_back(ptr[index + 1]);
					data.pixels.push_back(ptr[index + 0]);
					data.pixels.push_back(ptr[index + 3]);
				}
			}

			data.width = size.first;
			data.height = size.second;
			m_thumbnailDatas.push_back(data);
		}

        m_printer->applySettings();
		m_plateInfo = m_printer->getPlateInfo();
	}

#if 0
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
#endif // 0

	void AnsycWorker::work(qtuser_core::Progressor* progressor)
	{
		EngineType engine_type = getEngineType();

		m_slicing = true;
		if (!needReSlice())
			return;

		emit prepareSlice(m_printer);

		prepareSliceData();

		m_sliceAttain = NULL;
		SliceInput input;
		if (m_slicedModels.size() == 0)
		{
			progressor->message("nothing to slice.");
		}
		else
		{
			produceSliceInput(input, m_printer, engine_type);
		}

		QSharedPointer<qtuser_core::ProgressorTracer> tracer(new FormatSlice(progressor));
		if (engine_type == EngineType::ET_ORCA)
		{
			tracer.reset(new Crslice2Tracer(progressor));
		}
		tracer->resetProgressScope(0.0f, 0.85f);
		SliceResultPointer result;
		if (input.canSlice())
		{
			applyAdditionalSliceParameter();
			result.reset(new gcode::SliceResult());
			result->setSliceName(generateSceneName().toLocal8Bit().data());
			result->inputBox = input.sceneBox();
			input.pictures = m_thumbnailDatas;
			input.plate = m_plateInfo;
			m_sliceAttain = new SliceAttain(result, SliceAttainType::sat_file);

			if (engine_type == EngineType::ET_ORCA)
			{
				QScopedPointer<AnsycSlicer> slicer(new DLLAnsycSlicerOrca());
				slicer->doSlice(input, *tracer, m_sliceAttain);
			}
			else
			{
				QScopedPointer<AnsycSlicer> slicer(new DLLAnsycSlicer52());
				slicer->doSlice(input, *tracer, m_sliceAttain);
			}

		}
		else
		{
			tracer->failed("can not slice@");
		}

		if (tracer->error())
		{
			m_failExceptionDesc = tracer->getFailReason();
			return;
		}

		if (result)
		{
			tracer->resetProgressScope(0.85f, 1.0f);
			//load temp gcode
			result->load(cxkernel::qString2String(m_sliceAttain->tempGCodeFileName()));

			//m_sliceAttain->preparePreviewData(&tracer);
			m_sliceAttain->build_paraseGcode(tracer.get());
			emit sliceBeforeSuccess(m_sliceAttain);

			tracer->resetProgressScope(1.0f, 1.0f);

			qDebug() << QString("Slice : SliceAttain build over . [%1]").arg(currentProcessMemory());
		}

		if (tracer->interrupt())
		{
			m_failExceptionDesc = "User Cancelled@";
		}
	}
}
