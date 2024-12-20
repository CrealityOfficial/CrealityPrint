#include "cxgcode/us/defaultloader.h"
#include "cxgcode/us/usettings.h"

#include <QtCore/QFile>
#include <QtCore/QDebug>
#include <QFileInfo>

#include "cxgcode/us/settingdef.h"

namespace cxgcode
{
	DefaultLoader::DefaultLoader(QObject* parent)
		:QObject(parent)
	{
		m_defRecommendSetting = new USettings(this);
	}

	DefaultLoader::~DefaultLoader()
	{
	}

	void DefaultLoader::setConfigRoot(const QString& root)
	{
		m_configRoot = root;
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
	}
}
