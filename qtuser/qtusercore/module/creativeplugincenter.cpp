#include "creativeplugincenter.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QPluginLoader>
#include <QtCore/QDebug>

#include "qtusercore/string/resourcesfinder.h"
namespace qtuser_core
{
	CreativePluginCenter::CreativePluginCenter(QObject* parent)
		:QObject(parent)
	{
	}
	
	CreativePluginCenter::~CreativePluginCenter()
	{
	}
	void CreativePluginCenter::load()
	{
		QStringList searchPathes = QCoreApplication::libraryPaths();
		searchPathes.removeDuplicates();
		QStringList nameFilters = qtuser_core::dynamicLoadFilters();
		qDebug() << "CreativePluginCenter nameFilters : " << nameFilters;
		qDebug() << "CreativePluginCenter searchPathes : " << searchPathes;
		for (const QString& path : searchPathes)
		{
			QDir dir(path);
			QList<QFileInfo> fileInfos = dir.entryInfoList(nameFilters, QDir::Files);
			QString log = QString("CreativePluginCenter try find in path: [%1]->[%2]").arg(path).arg(fileInfos.size());
			qDebug() << log; 
			for (const QFileInfo& fileInfo : fileInfos)
			{
				QPluginLoader loader(fileInfo.filePath());
				if (loader.load())
				{
					QObject* object = loader.instance();
					{
						CreativeInterface* interf = qobject_cast<CreativeInterface*>(object);
						if (interf)
						{
							object->setParent(this);
	                        m_interfaces.insert(fileInfo.baseName(),interf);
						}else
							qDebug() << "Can't Find Interface in Plugin." << fileInfo.filePath();
					}
					qDebug() << fileInfo.filePath() << " QPluginLoader sucess.";
				}
				else
				{
					qDebug() << fileInfo.filePath() << " QPluginLoader Error." << loader.errorString();
				}
			}
		}
	}
	
	void CreativePluginCenter::initialize()
	{
	    for (CreativeInterface* interf : m_interfaces.values())
		{
			interf->initialize();
		}
	}
	
	void CreativePluginCenter::uninitialize()
	{	
	    for (CreativeInterface* interf : m_interfaces.values())
		{
			interf->uninitialize();
		}
	
		qDeleteAll(m_interfaces);
	}
	
	CreativeInterface* CreativePluginCenter::getPluginByName(QString name)
	{
	    if(m_interfaces.contains(name))
	    {
	        return m_interfaces.value(name);
	    }
	    return nullptr;
	}

}

