#ifndef CREATIVE_KERNEL_CRSLICEADAPTOR_
#define CREATIVE_KERNEL_CRSLICEADAPTOR_
#include <QtCore/QObject>
#include "basickernelexport.h"
#include "external/us/settinggroupdef.h"
#include "crslice/base/parametermeta.h"
#include "crslice2/base/parametermeta.h"
#include "external/kernel/enginedatatype.h"

namespace creative_kernel
{
	class BASIC_KERNEL_API CrsliceAdaptor : public QObject
	{
		Q_OBJECT
	public:
		using SaveKeysJsonFunc = std::function<void(const std::vector<std::string>& keys, const std::string& fileName)>;
		using EngineVersionFunc = std::function<std::string(void)>;

		static CrsliceAdaptor& getInstance() {
			static CrsliceAdaptor instance;
			return instance;
		}

		CrsliceAdaptor(CrsliceAdaptor const&) = delete;
		CrsliceAdaptor(CrsliceAdaptor&&) = delete;
		CrsliceAdaptor& operator=(CrsliceAdaptor const&) = delete;
		CrsliceAdaptor& operator=(CrsliceAdaptor&&) = delete;
	protected:
		CrsliceAdaptor(QObject* parent = nullptr);
		~CrsliceAdaptor();

	public:
		void initialize();
		void setEngineType(const EngineType& type);

		void getSettingGroupDefs(QMap<QString, us::SettingGroupDef*>& settingGroupDefs);

		void saveKeysJson(const std::vector<std::string>& keys, const std::string& fileName);

		std::string engineVersion();
		void getMetaKeys(MetaGroup metaGroup, std::vector<std::string>& keys);

		template<typename MetasMap, typename ParameterMeta>
		void doGetSettingGroupDefs(MetasMap datas, QMap<QString, us::SettingGroupDef*>& settingGroupDefs);

	private:
		EngineType m_engineType;
		SaveKeysJsonFunc m_saveKeysJsonFunc;
		EngineVersionFunc m_engineVersionFunc;
	};

	template<>
	inline void CrsliceAdaptor::doGetSettingGroupDefs<crslice::MetasMap, crslice::ParameterMeta>(
			crslice::MetasMap datas, QMap<QString, us::SettingGroupDef*>& settingGroupDefs)
	{
		us::SettingGroupDef* settingGroupDef = new us::SettingGroupDef(this);
		settingGroupDef->order = 0;
		settingGroupDef->key = QString("machine");

		for (auto it = datas.begin(); it != datas.end(); ++it)
		{
			crslice::ParameterMeta* meta = it->second;
			us::SettingItemDef* settingItemDef = new us::SettingItemDef(settingGroupDef);
			settingItemDef->name = meta->name.c_str();
			settingItemDef->label = meta->label.c_str();
			settingItemDef->description = meta->description.c_str();
			settingItemDef->type = meta->type.c_str();
			settingItemDef->defaultStr = meta->default_value.c_str();
			settingItemDef->valueStr = meta->value.c_str();
			settingItemDef->enableStatus = meta->enabled.c_str();
			if (settingItemDef->enableStatus.isEmpty())
			{
				settingItemDef->enableStatus = "true";
			}
			settingItemDef->unit = QString::fromLatin1(meta->unit.c_str());
			settingItemDef->minimum = meta->minimum_value.c_str();
			settingItemDef->maximum = meta->maximum_value.c_str();
			settingItemDef->miniwarning = meta->minimum_value_warning.c_str();
			settingItemDef->maxwarning = meta->maximum_value_warning.c_str();

			settingItemDef->settableGlobally = QStringLiteral("false");
			settingItemDef->settablePerExtruder = QStringLiteral("false");
			settingItemDef->settablePerMesh = meta->settable_per_mesh.c_str();
			settingItemDef->settablePerMeshGroup = QStringLiteral("false");

			for (const auto& optionIt : meta->options)
			{
				settingItemDef->options.push_back(std::make_pair(optionIt.first.c_str(), optionIt.second.c_str()));
			}
			settingGroupDef->addSettingItemDef(settingItemDef);
		}
		settingGroupDefs.insert(settingGroupDef->key, settingGroupDef);
	}

	template<>
	inline void CrsliceAdaptor::doGetSettingGroupDefs<crslice2::MetasMap, crslice2::ParameterMeta>(
			crslice2::MetasMap datas, QMap<QString, us::SettingGroupDef*>& settingGroupDefs)
	{
		us::SettingGroupDef* settingGroupDef = new us::SettingGroupDef(this);
		settingGroupDef->order = 0;
		settingGroupDef->key = QString("machine");

		for (auto it = datas.begin(); it != datas.end(); ++it)
		{
			crslice2::ParameterMeta* meta = it->second;
			us::SettingItemDef* settingItemDef = new us::SettingItemDef(settingGroupDef);
			settingItemDef->name = meta->name.c_str();
			settingItemDef->label = meta->label.c_str();
			settingItemDef->description = meta->description.c_str();
			settingItemDef->type = meta->type.c_str();
			settingItemDef->defaultStr = meta->default_value.c_str();
			settingItemDef->valueStr = meta->value.c_str();
			settingItemDef->enableStatus = meta->enabled.c_str();
			if (settingItemDef->enableStatus.isEmpty())
			{
				settingItemDef->enableStatus = "true";
			}
			settingItemDef->unit = QString::fromLatin1(meta->unit.c_str());
			settingItemDef->minimum = meta->minimum_value.c_str();
			settingItemDef->maximum = meta->maximum_value.c_str();
			settingItemDef->miniwarning = meta->minimum_value_warning.c_str();
			settingItemDef->maxwarning = meta->maximum_value_warning.c_str();

			settingItemDef->settableGlobally = meta->settable_globally.c_str();
			settingItemDef->settablePerExtruder = meta->settable_per_extruder.c_str();
			settingItemDef->settablePerMesh = meta->settable_per_mesh.c_str();
			settingItemDef->settablePerMeshGroup = meta->settable_per_meshgroup.c_str();

			for (const auto& optionIt : meta->options)
			{
				settingItemDef->options.push_back(std::make_pair(optionIt.first.c_str(), optionIt.second.c_str()));
			}
			settingGroupDef->addSettingItemDef(settingItemDef);
		}
		settingGroupDefs.insert(settingGroupDef->key, settingGroupDef);
	}
}

#endif
