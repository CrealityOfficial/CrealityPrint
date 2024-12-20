#include "crsliceadaptor.h"
#include "qtusercore/util/settings.h"
#include "rapidjson/document.h"
#include <QFile>
#include "internal/parameter/parameterpath.h"

namespace creative_kernel
{
	CrsliceAdaptor::CrsliceAdaptor(QObject* parent)
		: QObject(parent),
		m_engineType(EngineType::ET_CURA),
		m_saveKeysJsonFunc(&crslice::saveKeysJson),
		m_engineVersionFunc(&crslice::engineVersion)
	{
		qtuser_core::VersionSettings setting;
		setEngineType(static_cast<EngineType>(setting.value("engine_type", 0).toInt()));
	}

	CrsliceAdaptor::~CrsliceAdaptor()
	{

	}

	void CrsliceAdaptor::initialize()
	{
	}

	void CrsliceAdaptor::setEngineType(const EngineType& type)
	{
		if (m_engineType == type)
		{
			return;
		}
		m_engineType = type;
		switch (type)
		{
		case EngineType::ET_CURA:
		{
			m_engineVersionFunc = &crslice::engineVersion;
			m_saveKeysJsonFunc = &crslice::saveKeysJson;
		}
			break;
		case EngineType::ET_ORCA:
		{
			m_engineVersionFunc = &crslice2::engineVersion;
			m_saveKeysJsonFunc = &crslice2::saveKeysJson;
		}
			break;
		default:
			break;
		}
	}

	void CrsliceAdaptor::getSettingGroupDefs(QMap<QString, us::SettingGroupDef*>& settingGroupDefs)
	{
		switch (m_engineType)
		{
		case EngineType::ET_CURA:
		{
			crslice::MetasMap datas;
			crslice::parseMetasMap(datas);
			doGetSettingGroupDefs<crslice::MetasMap, crslice::ParameterMeta>(datas, settingGroupDefs);
		}
		break;
		case EngineType::ET_ORCA:
		{
			crslice2::MetasMap datas;
			crslice2::parseMetasMap(datas);
			doGetSettingGroupDefs<crslice2::MetasMap, crslice2::ParameterMeta>(datas, settingGroupDefs);
			QFile file(_pfpt_default_root + "/validate.json");
			QString currentClassType = "";
			if (file.open(QIODevice::ReadOnly | QIODevice::Text))
			{
				QByteArray barray = file.readAll();

				rapidjson::Document doc;
				doc.ParseInsitu(barray.data());
				if (doc.HasParseError())
				{
					qDebug() << "Json parse error";
					return;
				}
				for (us::SettingGroupDef* groupDef : settingGroupDefs)
				{
					for (us::SettingItemDef* itemDef : groupDef->items)
					{
						auto key = itemDef->name.toStdString();
						if (doc.HasMember(key.c_str()) && doc[key.c_str()].IsObject())
						{
							const auto& obj = doc[key.c_str()].GetObject();
							if (obj.HasMember("enabled") && obj["enabled"].IsString())
							{
								itemDef->enableStatus = obj["enabled"].GetString();
							}
							if (obj.HasMember("minimum_value") && obj["minimum_value"].IsString())
							{
								itemDef->minimum = obj["minimum_value"].GetString();
							}
							if (obj.HasMember("maximum_value") && obj["maximum_value"].IsString())
							{
								itemDef->maximum = obj["maximum_value"].GetString();
							}
							if (obj.HasMember("miniwarning") && obj["miniwarning"].IsString())
							{
								itemDef->miniwarning = obj["miniwarning"].GetString();
							}
							if (obj.HasMember("minimum_value_warning") && obj["minimum_value_warning"].IsString())
							{
								itemDef->miniwarning = obj["minimum_value_warning"].GetString();
							}
							if (obj.HasMember("maxwarning") && obj["maxwarning"].IsString())
							{
								itemDef->maxwarning = obj["maxwarning"].GetString();
							}
							if (obj.HasMember("maximum_value_warning") && obj["maximum_value_warning"].IsString())
							{
								itemDef->maxwarning = obj["maximum_value_warning"].GetString();
							}
							if (obj.HasMember("value") && obj["value"].IsString())
							{
								itemDef->valueStr = obj["value"].GetString();
							}
							if (obj.HasMember("default_value") && obj["default_value"].IsString())
							{
								itemDef->defaultStr = obj["default_value"].GetString();
							}
							if (obj.HasMember("settable_per_mesh") && obj["settable_per_mesh"].IsString())
							{
								itemDef->settablePerMesh = obj["settable_per_mesh"].GetString();
							}
							if (obj.HasMember("settable_per_meshgroup") && obj["settable_per_meshgroup"].IsString())
							{
								itemDef->settablePerMeshGroup = obj["settable_per_meshgroup"].GetString();
							}
						}
					}
				}
			}
		}
		break;
		default:
			break;
		}
	}

	void CrsliceAdaptor::saveKeysJson(const std::vector<std::string>& keys, const std::string& fileName)
	{
		if (m_saveKeysJsonFunc)
		{
			m_saveKeysJsonFunc(keys, fileName);
		}
	}

	std::string CrsliceAdaptor::engineVersion()
	{
		if (m_engineVersionFunc)
		{
			return m_engineVersionFunc();
		}
		return {};
	}

	void CrsliceAdaptor::getMetaKeys(MetaGroup metaGroup, std::vector<std::string>& keys)
	{
		switch (m_engineType)
		{
		case EngineType::ET_CURA:
		{
			crslice::getMetaKeys(static_cast<crslice::MetaGroup>(metaGroup), keys);
		}
		break;
		case EngineType::ET_ORCA:
		{
			crslice2::getMetaKeys(static_cast<crslice2::MetaGroup>(metaGroup), keys);
		}
		break;
		default:
			break;
		}
	}

}
