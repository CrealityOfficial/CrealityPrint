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

namespace us
{
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
		qDebug() << QString("DefaultLoader::loadDefault : [%1]").arg(fileName);

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
			}
			if (metadata.contains("id") && metadata.value("id").isString())
			{
				material_id = metadata.value("id").toString();
				if (!material_id.isEmpty())
				{
					USetting* setting = SETTING2("filament_ids", material_id);
					uSettings->insert(setting);
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
					USetting* setting = SETTING2(key, engine_data.value(key).toString());
					uSettings->insert(setting);
				}
			}
		}
		if (rootObj.contains("printer") && rootObj.value("printer").isObject())
		{
			QJsonObject engine_data = rootObj.value("printer").toObject();
			for (const auto& key : engine_data.keys())
			{
				USetting* setting = SETTING2(key, engine_data.value(key).toString());
				uSettings->insert(setting);
			}
		}
		if (rootObj.contains("extruders") && rootObj.value("extruders").isArray())
		{
			QJsonArray extruder_data = rootObj.value("extruders").toArray();
			for (const auto& type_printer_ref : extruder_data)
			{
				USettings* settings = new USettings();
				QJsonObject engine_data = type_printer_ref.toObject().value("engine_data").toObject();
				for (const auto& key : engine_data.keys())
				{
					USetting* setting = SETTING2(key, engine_data.value(key).toString());
					settings->insert(setting);
				}
				if (extruderSettings)
				{
					extruderSettings->push_back(settings);
				}
			}
		}
	}
}
