#include "unittesttool.h"
#include <QDebug>
#include <QFile>
#include <QCoreApplication>
#include <QSvgGenerator>
#include <QSettings>
#include <QStandardPaths>
#include <QtCore/qprocess.h>
#include <QDir>
//#pragma execution_character_set("utf-8")

QVariant BaseSettings::getSettingValue(const QString& key, const QVariant& defaultValue)
{
	QSettings setting;
	setting.beginGroup("too_baseline_config");
	QVariant val = setting.value(key, defaultValue);
	setting.endGroup();

	return val;
}

void BaseSettings::setSettingValue(const QString& key, const QVariant& value)
{
	QSettings setting;
	setting.beginGroup("too_baseline_config");
	setting.setValue(key, value);
	setting.endGroup();
}

bool readErrorFiles(const QString dirPath, QString& resultMessage)
{
	// 获取所有文件和目录
	QDir dir(dirPath);
	if (!dir.exists())return false;
	QFileInfoList list = dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot, QDir::Name);
	for (auto fileInfo : list)
	{
		qDebug() << fileInfo.fileName();
		if (fileInfo.isFile()) {
			qDebug() << "Found file:" << fileInfo.absoluteFilePath();
			//cxkernel::openFileWithName(fileInfo.absoluteFilePath());
			if (fileInfo.suffix() == "errtxt")
			{
				resultMessage += fileInfo.baseName().left(fileInfo.baseName().lastIndexOf("_compare")) + " file error :\n";
				loadText(fileInfo.absoluteFilePath(),resultMessage);
			}
			else
			{
				continue;
			}
		}
	}
	return true;

	
}

void loadText(const QString& fileName,QString &message)
{
	QFile qFile(fileName);
	if (!qFile.open(QIODevice::ReadOnly))
	{
		qDebug() << QString("loadSliceProfile [%1] failed.").arg(fileName);
		return;
	}
	while (qFile.atEnd() == false) {
		QString line = qFile.readLine().simplified();
		//if (line.isEmpty()) {
		//	continue;
		//}
		if (line.contains("error:"))
		{
			message	+= line + "\n";
		}
		else
		{
			continue;
		}
		//QStringList words = line.split(":");
		//if (words.size() > 1) {

		//}
	}
	qFile.close();
}

UnitTestTool::UnitTestTool(QObject* parent) : QObject(parent)
, m_qmlEngine(nullptr)
{
	m_translator = new QTranslator(this);

	const QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
	m_slicerBLPath = BaseSettings::getSettingValue("tool_slicer_baseline_path", QString("%1/").arg(defaultPath)).toString();
	m_slicerComparePath = BaseSettings::getSettingValue("tool_slicer_compare_path", QString("%1/").arg(defaultPath)).toString();
	m_3mfPath = BaseSettings::getSettingValue("tool_3mf_path", QString("%1/").arg(defaultPath)).toString();
	m_mess_text = "123\n" + QString("789\n");
}

UnitTestTool::~UnitTestTool() {

}
void UnitTestTool::initTranslator()
{
	QString language_file_name = "en.ts";
	m_translator->load(QStringLiteral(":/language/%1").arg(language_file_name));
	QCoreApplication::installTranslator(m_translator);
}

void UnitTestTool::setQmlEngine(QQmlEngine* engine)
{
	m_qmlEngine = engine;
}

void UnitTestTool::changeLanguage(int index)
{
	QString file = "";
	switch (index)
	{
	case 0:
		file = QStringLiteral(":/language/%1").arg("en.ts");
		break;
	case 1:
		file = QStringLiteral(":/language/%1").arg("zh_CN.ts");
		break;
	default:
		break;
	}

	if (m_translator->load(file))
	{
		//QCoreApplication::removeTranslator(m_translator);
		//QCoreApplication::installTranslator(m_translator);
		if (m_qmlEngine)
			m_qmlEngine->retranslate();
		return;
	}
}

QString UnitTestTool::slicer3MFPath() const
{
	return m_3mfPath;
}

void UnitTestTool::set3MFPath(const QString& strPath)
{
	m_3mfPath = strPath;
	BaseSettings::setSettingValue("tool_3mf_path", strPath);
	Q_EMIT mfPathChanged();
}

QString UnitTestTool::slicerBLPath() const
{
	return m_slicerBLPath;
}
void UnitTestTool::setSlicerBLPath(const QString& strPath)
{
	m_slicerBLPath = strPath;
	BaseSettings::setSettingValue("tool_slicer_baseline_path", strPath);
	Q_EMIT slicerBLPathChanged();
}

QString UnitTestTool::compareSliceBLPath()const
{
	return m_slicerComparePath;
}
void UnitTestTool::setCompareSlicerBLPath(const QString& strPath)
{
	m_slicerComparePath = strPath;
	BaseSettings::setSettingValue("tool_slicer_compare_path", strPath);
	Q_EMIT slicerComparePathChanged();
}
QString UnitTestTool::message() const
{
	return m_mess_text;
}
void UnitTestTool::doTest(const int& type)
{
	//-bltest --3mfpath "F:\3MFPath" --blpath F:\BLTEST --cmppath "F:\COMPARETEST"
	QStringList cmdList;
	cmdList << "CrealityPrint.exe";
	cmdList << "-bltest";
	cmdList << "--3mfpath";
	cmdList << m_3mfPath;
	cmdList << "--blpath";
	cmdList << m_slicerBLPath;
	cmdList << "--cmppath";
	cmdList << m_slicerComparePath;
	switch (type)
	{
	case 0:
		cmdList << "-generate";
		break;
	case 1:
		cmdList << "-compare";
		break;
	case 2:
		cmdList << "-update";
		break;
	default:
		cmdList << "-compare";
		break;
	}
	const auto program = cmdList.takeFirst();

	QDir dir(m_slicerComparePath);
	dir.removeRecursively();


	//清空描述
	m_mess_text.clear();

	int ret = QProcess::execute(program, cmdList);

	if (ret == 0)
	{
		qDebug() << "process end";
		m_mess_text += "process end \n";
		readErrorFiles(m_slicerComparePath, m_mess_text);
		emit messageChanged();
	}
	
}
