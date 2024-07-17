#include "preferencemanager.h"
#include "kernel/visualscene.h"
#include "interface/camerainterface.h"
#include <QSettings>
#include <QStandardPaths>
#include <QDir>
#include "qtusercore/string/resourcesfinder.h"
#include "interface/spaceinterface.h"
#include <QString>
#include <QFileInfo>
#include "buildinfo.h"
#include "external/interface/uiinterface.h"
#include "qtusercore/module/systemutil.h"
// compatibility with resource files of v4.3.
#define OLD_PROJECT_NAME "Creative3D"
namespace creative_kernel
{
	bool removeDir(const QString& path,const QString skipDir)
	{
		if (path.isEmpty())
			return false;

		QDir dir(path);
		if (!dir.exists())
			return false;

		dir.setFilter(QDir::AllEntries | QDir::NoDotAndDotDot);
		QFileInfoList fileList = dir.entryInfoList();
		bool chlidDir = false;
		for (const QFileInfo& file : fileList)
		{
			if (file.isFile())
			{
				file.dir().remove(file.fileName());
			}
			else
			{
				if (skipDir == file.absoluteFilePath())
				{
					chlidDir = true;
					continue;
				}
				clearPath(file.absoluteFilePath());
				file.dir().rmdir(file.absoluteFilePath());
			}
		}
		if (!chlidDir)
		{
			dir.rmdir(path);
		}

		return true;
	}
	PreferenceManager::PreferenceManager(QObject* parent)
		:QObject(parent)
	{
		QString realCloudPath(QFileInfo{
			qtuser_core::getOrCreateAppDataLocation(QStringLiteral("../../%1_%2").arg(QStringLiteral(OLD_PROJECT_NAME)).arg(QStringLiteral("cloud_models")))
			}.absoluteFilePath());
		m_downloadModelPath = getSettingValue("download_model_path", realCloudPath).toString();
	}

	PreferenceManager::~PreferenceManager()
	{

	}



	void PreferenceManager::setDownloadModelPath(const QString path)
	{
		m_lastModelPath = m_downloadModelPath;
		m_downloadModelPath = path;
		setSettingValue("download_model_path", path);
		Q_EMIT downPathChanged();
		if (m_lastModelPath != m_downloadModelPath)
		{
			swithPathDialog();
		}
	}
	void PreferenceManager::swithPathDialog()
	{
		creative_kernel::requestQmlDialog("switchDownModelPathDialog");
	}
	void PreferenceManager::acceptRepalceCache()
	{
		QString lastModelCache = QString(m_lastModelPath + "/cache");
		QString downModelCache = QString(m_downloadModelPath + "/cache");
		bool bRet = qtuser_core::copyDir(lastModelCache, downModelCache,true);
		QString lastModelCacheInfoJson = QString(m_lastModelPath + "/cache_info.json");
		QString downModelCacheInfoJson = QString(m_downloadModelPath + "/cache_info.json");
		bRet = qtuser_core::copyFile(lastModelCacheInfoJson, downModelCacheInfoJson, true);
		if (bRet)
		{
			removeDir(lastModelCache, downModelCache);
			qtuser_core::removeFile(lastModelCacheInfoJson);
			creative_kernel::requestQmlDialog("migrateSuccessful");
		}
		
	}
	
	QVariant PreferenceManager::getSettingValue(const QString& key, const QVariant& defaultValue)
	{
		QSettings setting;
		setting.beginGroup("perferences_config");
		QVariant val = setting.value(key, defaultValue);
		setting.endGroup();

		return val;
	}

	void PreferenceManager::setSettingValue(const QString& key, const QVariant& value)
	{
		QSettings setting;
		setting.beginGroup("perferences_config");
		setting.setValue(key, value);
		setting.endGroup();
	}

}