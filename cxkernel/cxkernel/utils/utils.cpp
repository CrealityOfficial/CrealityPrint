#include "utils.h"
#include <QtCore/QDir>

#include "cxkernel/utils/anonymousjob.h"
#include "cxkernel/interface/jobsinterface.h"
#include "qtusercore/string/resourcesfinder.h"

#include <locale>
#include <codecvt>
#include <QStandardPaths>
#include <QDateTime>
#include "qtusercore/string/stringtool.h"
namespace cxkernel
{
	void circleDirectory(const QString& directory, circleLoadFunc func)
	{
		QList<QString> fileNames;

		QDir dir(directory);
		QList<QFileInfo> fileInfos = dir.entryInfoList(QStringList(), QDir::Files);

		for (const QFileInfo& fileInfo : fileInfos)
		{
			if (fileInfo.isFile())
				fileNames << fileInfo.absoluteFilePath();
		}

		for (const QString& fileName : fileNames)
			func(fileName, nullptr);
	}

	void ansycBatch(const QString& directory, circleLoadFunc func)
	{
		QStringList fileNames = qtuser_core::allFiles(directory);
		auto workFunc = [fileNames, func](ccglobal::Tracer* tracer) {

			int count = fileNames.count();
			for (int i = 0; i < count; ++i)
			{
				if (tracer)
				{
					float delta = 1.0f / (float)count;
					tracer->resetProgressScope((float)i * delta, (float)(i + 1) * delta);
				}

				const QString& fileName = fileNames.at(i);

				qDebug() << QString("SimulationFlow::ansycBatch start process [%1]").arg(fileName);
				func(fileName, tracer);
			}
		};

		auto successFunc = []() {
			qDebug() << QString("Anonymous job success.");
		};

		qDebug() << QString("Anonymous job start.");
		AnonymousJob* job = new AnonymousJob(workFunc, successFunc, nullptr);
		cxkernel::executeJob(job);
	}

	void runAnonymous(anonymous_work_func workFunc, anonymous_func successFunc)
	{
		AnonymousJob* job = new AnonymousJob(workFunc, successFunc, nullptr);
		cxkernel::executeJob(job);
	}

	std::string qString2String(const QString& str)
	{
		std::wstring wstr = str.toStdWString();

		using convert_typeX = std::codecvt_utf8<wchar_t>;
		std::wstring_convert<convert_typeX, wchar_t> converterX;

		return converterX.to_bytes(wstr);
	}
	void clearModelSerializeData()
	{
		QString strPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/RecentSerializeModel/";
		QDir d(strPath);
		if (!d.exists())
		{
			return;
		}

		d.removeRecursively();
	}
	QString createPureName(const QString& name)
	{
		return qtuser_core::qstringMd5(name);
	}
	QString getSerializeModelName(QString model_name)
	{
		QString strPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/RecentSerializeModel/";
		QDir d(strPath);
		if (!d.exists())
		{
			d.mkdir(strPath);
		}

		QDateTime current_date_time = QDateTime::currentDateTime();
		QString current_date_str = current_date_time.toString("yyyy.MM.dd hh:mm:ss.zzz");
		QString pure_name = createPureName(model_name+ current_date_str);

		return strPath + pure_name + ".ser";
	}
}