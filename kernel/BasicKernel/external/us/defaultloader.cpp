#include "us/defaultloader.h"
#include "us/usettings.h"

#include <QtCore/QFile>
#include <QtCore/QDebug>
#include <QFileInfo>

#include "us/settingdef.h"
#include "qcxutil/crypt/cryptfiledevice.h"
#include "interface/appsettinginterface.h"

namespace us
{
	DefaultLoader::DefaultLoader(QObject* parent)
		:QObject(parent)
	{
		m_configRoot = DEFAULT_CONFIG_ROOT;
		m_defRecommendSetting = new USettings();
	}

	DefaultLoader::~DefaultLoader()
	{
		if (m_defRecommendSetting != nullptr)
		{
			delete m_defRecommendSetting;
			m_defRecommendSetting = nullptr;
		}
	}

	void DefaultLoader::loadDefault(const QString& fileName, USettings* uSettings)
	{
		if (uSettings != NULL)
		{
			uSettings->clear();
		}
       
		if (true)
		{
			QFile file(fileName);
			qDebug() << QString("DefaultLoader::loadDefault : [%1]").arg(fileName);
			if (file.open(QIODevice::ReadOnly | QIODevice::Text))
			{
				while (!file.atEnd())
				{
					QString line = file.readLine().trimmed();
					if (line.isEmpty() || 
						(line == "\n") ||
						(line.contains("[") && line.contains("]") && !line.contains("="))
						)
						continue;

					QStringList lists = line.split("=");
#if 0 //_DEBUG
					qDebug() << lists;
#endif
					if (lists.size() == 1)
					{
						USetting* setting = SETTING(lists.at(0).trimmed());
						uSettings->insert(setting);
					}
					else if (lists.size() > 2) //start_gcode÷– ∫¨”– =
					{
						for (int n = 2; n < lists.size(); n++)
						{
							lists[1] += "=" + lists[n];
						}
						lists[1].replace("\"", "");
						USetting* setting = SETTING2(lists.at(0).trimmed(), lists.at(1).trimmed());
						uSettings->insert(setting);
					}
					else
					{
						USetting* setting = SETTING2(lists.at(0).trimmed(), lists.at(1).trimmed());
						uSettings->insert(setting);
					}
				}
			}
			file.close();
		}
		else //decrypt
		{
			qcxutil::CryptFileDevice pFile;
			pFile.setFileName(fileName);
			if (pFile.open(QIODevice::ReadOnly | QIODevice::Text))
			{
				while (!pFile.atEnd())
				{
					QByteArray tmp = pFile.readLine();
					QString line = QString(tmp);
					QStringList lists = line.split("=");
					if (lists.size() == 1)
					{
                        if (lists[0] != "\n")//empty
                        {
                            USetting* setting = SETTING(lists.at(0).trimmed());
                            uSettings->insert(setting);
                        }
					}
					else if (lists.size()>2)
					{
						 for (int n=2;n<lists.size();n++)
						 {
							 lists[1] += "="+lists[n];
						 }
						 lists[1].replace("\"", "");
						 USetting* setting = SETTING2(lists.at(0).trimmed(), lists.at(1).trimmed());
						 uSettings->insert(setting);
					}
					else
					{
						lists[1].replace("\"","");
						USetting* setting = SETTING2(lists.at(0).trimmed(), lists.at(1).trimmed());
						uSettings->insert(setting);
					}
				}
			}
			pFile.close();

			//Optimize the configuration file loading method
			QFileInfo fileInfo(fileName);
			QString defFile = "";
			if (!fileInfo.fileName().isEmpty())
			{
				defFile = QString("%1/%2").arg(m_configRoot).arg(fileInfo.fileName());
			}

			m_defRecommendSetting->clear();
			QFile tmpFile(defFile);
			if (tmpFile.open(QIODevice::ReadOnly | QIODevice::Text))
			{
				while (!tmpFile.atEnd())
				{
					QString line = tmpFile.readLine();
					QStringList lists = line.split("=");
					if (lists.size() == 1)
					{
						USetting* setting = SETTING(lists.at(0).trimmed());
						m_defRecommendSetting->insert(setting);
					}
					else
					{
						USetting* setting = SETTING2(lists.at(0).trimmed(), lists.at(1).trimmed());
						m_defRecommendSetting->insert(setting);
					}
				}
			}
			tmpFile.close();
			uSettings->mergeNewItem(m_defRecommendSetting);
			//end
		}
	}
}
