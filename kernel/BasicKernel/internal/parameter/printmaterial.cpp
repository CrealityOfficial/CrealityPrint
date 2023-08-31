#include <QUrl>
#include <QFile>
#include "printmaterial.h"
#include "printmachine.h"
#include "printextruder.h"
#include "printprofile.h"
#include "internal/models/profileparametermodel.h"
#include "qtusercore/string/resourcesfinder.h"
//#include "internal/parameter/printmachine.cpp"

namespace creative_kernel
{
	PrintMaterial::PrintMaterial()
	{

	}

	PrintMaterial::PrintMaterial(const QString& machineName, const MaterialData& data, QObject* parent)
		: ParameterBase(parent)
		, m_machineName(machineName)
		, m_data(data)
        , m_materialParameterModel(nullptr)
	{
		//m_name = m_fileName.mid(0, m_fileName.indexOf(".default"));
	}

	PrintMaterial::~PrintMaterial()
	{

	}

	QString PrintMaterial::name()
	{
		return m_data.name;
		//return m_data.uniqueName();
	}

	QString PrintMaterial::brand()
	{
		return m_data.brand;
	}

	QString PrintMaterial::type()
	{
		return m_data.type;
	}

	Q_INVOKABLE bool PrintMaterial::isUserDef()
	{
		return m_data.isUserDef;
	}

	void PrintMaterial::setName(const QString& materialName)
	{
		m_data.name = materialName;
	}

    QString PrintMaterial::uniqueName()
    {
        return m_data.uniqueName();
    }

	void PrintMaterial::added()
	{
		setSettings(createMaterialSettings(m_machineName, m_data));
		setUserSettings(createUserMaterialSettings(m_machineName, m_data));
		const QStringList qualityString = { "high","middle","low" };

		for (const auto& quality : qualityString)
		{
			us::USettings* settings = createMaterialOverrideSettings(m_machineName, m_data, quality);
			if (settings)
			{
				m_overrideSettings[quality] = settings;
			}
		}
	}

	void PrintMaterial::removed()
	{
		//1.ÒÆ³ýÎÄ¼þ
		QString filePath = userMaterialFile(m_machineName, m_data.uniqueName());
		QFile file(filePath);
		if (!file.exists())
			return;
		file.remove();
	}

	void PrintMaterial::addedUserMaterial()
	{
		setSettings(createMaterialSettings(m_machineName, m_data));
		setUserSettings(createUserMaterialSettings(m_machineName, m_data));
	}

	QAbstractListModel* PrintMaterial::materialParameterModel(const int& extruderIndex, bool professional)
    {
		if (!m_settings)
		{
			return nullptr;
		}
		if (!m_materialParameterModel)
		{
			m_materialParameterModel = new ProfileParameterModel(m_settings, this);
			PrintMachine* machine = qobject_cast<PrintMachine*>(this->parent());
			PrintExtruder* extruder = qobject_cast<PrintExtruder*>(machine->extruderObject(extruderIndex));
			ProfileParameterModel* model = qobject_cast<ProfileParameterModel*>(extruder->extruderParameterModel("retraction", true));
			QList<us::USetting*> filter_settings = model->filtterSettings();
			auto global = machine->createCurrentGlobalSettings();
			auto override_keys = loadMaterialKeys("override");
			for (const auto& key : override_keys)
			{
				if (m_settings->hasKey(key))
				{
					if (!m_user_settings->hasKey(key))
					{
						m_user_settings->insert(m_settings->settings().value(key));
					}
				}
				else
				{
					bool found = false;
					for (auto filter : filter_settings)
					{
						if (filter->key() == key)
						{
							found = true;
							m_settings->insert(filter->clone());
						}
					}
					//if (!found)
					//{
					//	m_settings->insert(global.get()->findSetting(key)->clone());
					//}
				}
			}
			m_materialParameterModel->applyUserSetting(m_user_settings);
		}
            
        return m_materialParameterModel;
    }

	Q_INVOKABLE void PrintMaterial::exportMaterialFromQml(const QString& urlString)
	{
		QUrl url(urlString);
		this->exportSetting(url.toLocalFile());
	}

	void PrintMaterial::save()
	{
		saveSetting(userMaterialFile(m_machineName, m_data.uniqueName()));
	}

	void PrintMaterial::cancel()
	{
	}

	void PrintMaterial::reset()
	{
		if (m_materialParameterModel)
		{
			QList<QString> keys;
			QHash<QString, us::USetting* >::const_iterator it = m_user_settings->settings().constBegin();
			while (it != m_user_settings->settings().constEnd())
			{
				keys.append(it.key());
				it++;
			}
			for (int i = 0; i < keys.size(); i++)
			{
				m_materialParameterModel->resetValue(keys[i]);
			}
		}
		save();
	}
	us::USettings* PrintMaterial::getOverrideSettings(const QString& quality)
	{
		QList<us::USetting*> coverUser_settings = static_cast<PrintMachine*>(this->parent())->currentMaterialCover();
		us::USettings* settings = new us::USettings;
		for (us::USetting* setting : coverUser_settings)
		{
			settings->insert(setting);
		}

		return settings;
	/*	if (m_overrideSettings.find(quality) != m_overrideSettings.cend())
		{
			return m_overrideSettings[quality];
		}
		return nullptr;*/
	}
}
