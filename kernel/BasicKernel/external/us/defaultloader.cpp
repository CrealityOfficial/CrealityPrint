#include "us/defaultloader.h"
#include "us/usettings.h"

#include <QtCore/QFile>
#include <QtCore/QDebug>
#include <QFileInfo>

#include "us/settingdef.h"
#include "interface/appsettinginterface.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "internal/parameter/parameterpath.h"

#define CHECK_VALUE_BEFORE_INSERT

namespace us
{
	namespace
	{

		bool TryInsertSetting(USettings* settings, const QString& key, const QString& value)
		{
			if (!settings)
			{
				return false;
			}

			USetting* setting = SETTING(key);
			if (!setting)
			{
				return false;
			}
			if ((setting->type() == QStringLiteral("coBool") || setting->type() == QStringLiteral("coBools")) && value.compare("true", Qt::CaseInsensitive) == 0)
			{
				setting->setValue("1");
			}
			else
			{
				setting->setValue(value);
			}

#if defined(CHECK_VALUE_BEFORE_INSERT)
			if (setting->type() != QStringLiteral("coString") &&
					setting->type() != QStringLiteral("coStrings"))
			{
				if (setting->str().isEmpty())
				{
					delete setting;
					return false;
				}
			}
#endif

			settings->insert(setting);
			return true;
		}

	}  // namespace

	DefaultLoader::DefaultLoader(QObject* parent)
		:QObject(parent)
	{
		m_configRoot = _pfpt_default_root;
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

	void DefaultLoader::loadDefault(const QString& fileName, USettings* uSettings, QList<USettings*>* extruderSettings, creative_kernel::MachineData* data)
	{
		QFile file(fileName);
		//qDebug() << QString("DefaultLoader::loadDefault : [%1]").arg(fileName);

		if (!file.open(QIODevice::ReadOnly)) {
			qDebug() << "File open failed!";
			return;
		}

		QJsonParseError error{ QJsonParseError::ParseError::NoError };
		QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &error);
		if (error.error != QJsonParseError::ParseError::NoError) {
			qDebug() << "parseJson:" << error.errorString();
			return;
		}
		if (document.isNull() || !document.isObject())
		{
			return;
		}
		file.close();
		QJsonObject rootObj = document.object();
		if (rootObj.contains("inherits") && rootObj.value("inherits").isString())
		{
			QString inherits = rootObj.value("inherits").toString();
			if (!inherits.isEmpty())
			{
				QString inherits_file = QString("%1/%2.json").arg(_pfpt_default_root).arg(inherits);
				loadDefault(inherits_file, uSettings, extruderSettings);
			}
		}
		QString material_id;
		if (rootObj.contains("metadata") && rootObj.value("metadata").isObject())
		{
			QJsonObject metadata = rootObj.value("metadata").toObject();
			if (data)
			{
				if (metadata.contains("is_bbl_printer") && metadata.value("is_bbl_printer").isBool())
				{
					data->is_bbl_printer = metadata.value("is_bbl_printer").toBool();
				}
				if (metadata.contains("preferred_process") && metadata.value("preferred_process").isString())
				{
					data->preferredProcess = metadata.value("preferred_process").toString();
				}
				if (metadata.contains("inherits_from") && metadata.value("inherits_from").isString())
				{
					data->inherits_from = metadata.value("inherits_from").toString();
				}
				if (metadata.contains("is_import") && metadata.value("is_import").isBool())
				{
					data->is_import = metadata.value("is_import").toBool();
				}
				if (metadata.contains("top_material") && metadata.value("top_material").isArray())
				{
					QJsonArray top_material_data = metadata.value("top_material").toArray();
					for (const auto& top_material_id : top_material_data)
					{
						data->top_material<< top_material_id.toString();
					}
				}
			}
			if (metadata.contains("id") && metadata.value("id").isString())
			{
				material_id = metadata.value("id").toString();
				if (!material_id.isEmpty())
				{
					TryInsertSetting(uSettings, "filament_ids", material_id);
				}
			}
		}
		if (rootObj.contains("engine_data") && rootObj.value("engine_data").isObject())
		{
			QJsonObject engine_data = rootObj.value("engine_data").toObject();
			for (const auto& key : engine_data.keys())
			{
				if (engine_data.value(key).isObject())
				{
					QJsonObject detail_engine_data = engine_data.value("key").toObject();
					USetting* setting = SETTING(key);
					if (rootObj.contains("value") && rootObj.value("value").isString())
					{
						setting->setValue(rootObj.value("value").toString());
					}
					uSettings->insert(setting);
				}
				else
				{
					auto value = engine_data.value(key).toString();
					if (key == "ensure_vertical_shell_thickness") {
						if (value == "0") {
						value = "ensure_moderate";
						} else if (value == "1") {
						value = "ensure_all";
						}
					}
					TryInsertSetting(uSettings, key, value);
				}
			}
		}
		if (rootObj.contains("printer") && rootObj.value("printer").isObject())
		{
			QJsonObject engine_data = rootObj.value("printer").toObject();
			for (const auto& key : engine_data.keys())
			{
				TryInsertSetting(uSettings, key, engine_data.value(key).toString());
			}
		}
		if (extruderSettings && rootObj.contains("extruders") && rootObj.value("extruders").isArray())
		{
			QJsonArray extruder_data = rootObj.value("extruders").toArray();
			for (const auto& type_printer_ref : extruder_data)
			{
				USettings* settings = new USettings();
				QJsonObject engine_data = type_printer_ref.toObject().value("engine_data").toObject();
				for (const auto& key : engine_data.keys())
				{
					TryInsertSetting(settings, key, engine_data.value(key).toString());
				}
				
				extruderSettings->push_back(settings);
			}
		}
	}
}
