#include "profileparametermodel.h"
#include "internal/parameter/printprofile.h"
#include "interface/uiinterface.h"
#include "internal/parameter/printmachine.h"
#include "internal/parameter/printextruder.h"
#include "internal/parameter/printmaterial.h"
#include <QCoreApplication>
namespace creative_kernel
{
	enum {
		NameRole = Qt::UserRole + 1,
		NameRole1,
		NameRole2,
		NameRole3,
		NameRole4,
		NameRole5,
		NameRole6,
		NameRole7,
		NameRole8,
		NameRole9,
		NameRole10,
		NameRole11,
		NameRole12,
		NameRole13
	};

	ProfileParameterModel::ProfileParameterModel(us::USettings* settings, QObject* parent)
		: QAbstractListModel(parent)
		, m_settings(settings)
	{
		if (parent && qobject_cast<PrintMaterial*>(parent))
		{
			m_userSettings = qobject_cast<PrintMaterial*>(parent)->userSettings();
		}
		m_global_settings = new us::USettings(this);
		m_global_settings->loadCompleted();
		m_searchSettings = new us::USettings(this);
	}

	ProfileParameterModel::~ProfileParameterModel()
	{
		if (m_global_settings)
		{
			delete m_global_settings;
			m_global_settings = nullptr;
		}
		if (m_searchSettings)
		{
			delete m_searchSettings;
			m_searchSettings = nullptr;
		}
	}

	void ProfileParameterModel::setFilter(const QString& category, const QString& filter, bool professional)
	{
		beginResetModel();
		if (!m_searchSettings->isEmpty()|| filter!="")
		{
			m_filterSettings = m_searchSettings->filter(category, "", professional);
		}
		else {
			m_filterSettings = m_settings->filter(category, filter, professional);
		}
		
		endResetModel();
	}
	Q_INVOKABLE void ProfileParameterModel::addSearchSettings(QObject* settings)
	{
		m_searchSettings->merge(qobject_cast<us::USettings*>(settings));
	}
	QAbstractListModel* ProfileParameterModel::searchCategoryModel(bool professional,bool isSingleExtruder, const QString& filter)
	{
		if (m_searchSettings->isEmpty() && filter!="")
		{
			//beginResetModel();
			//m_filterSettings.clear();
			//endResetModel();
			//return nullptr;
		}
		if (m_parameterProfileCategoryModel)
		{
			m_parameterProfileCategoryModel->deleteLater();
		}
		m_parameterProfileCategoryModel = new ParameterProfileCategoryModel(m_searchSettings, this, isSingleExtruder);
		beginResetModel();
		m_filterSettings = m_searchSettings->filter(m_parameterProfileCategoryModel->getFirtCategory().toLower(), "", professional);
		endResetModel();
		return m_parameterProfileCategoryModel;
	}
	Q_INVOKABLE QObject* ProfileParameterModel::search(const QString& category, const QString& filter, bool professional)
	{
		m_searchSettings->clear();
		if (filter == "")
		{
			beginResetModel();
			m_filterSettings = m_settings->filter(category, "", professional);
			endResetModel();
			return m_searchSettings;
		}
		QList<us::USetting*> settings;
		QHash<QString, us::USetting* >::const_iterator it = m_settings->settings().constBegin();
		
		while (it != m_settings->settings().constEnd())
		{
			us::SettingItemDef* def = it.value()->def();
			if ((professional && def->level <= 4)
				|| (!professional && def->level <= 2))
			{
				QString label = QCoreApplication::translate("ParameterGeneralCom", def->label.toLatin1().data());//invokeJS("qsTr('"+ def->label +"')", this).toString();//tr(def->label.toLatin1());
				if (label.toLower().contains(filter.toLower()))
				{
					QJSValue value = invokeJS(def->enableStatus, const_cast<ProfileParameterModel*>(this));
					bool enabled = value.toBool();
					if (enabled)
					{
						m_searchSettings->insert(it.value());
					}
					//settings.append(def);
				}
			}
			it++;
		}
		return m_searchSettings;
	}
	void ProfileParameterModel::setMaterialCategory(const QString& category)
	{
		if (!m_settings)
			return;
		beginResetModel();
		m_filterSettings = m_settings->materialParameters(category);
		endResetModel();
	}

	void ProfileParameterModel::setExtruderCategory(const QString& category, const bool& professional)
	{
		if (!m_settings)
			return;
		beginResetModel();
		m_filterSettings = m_settings->extruderParameters(category, professional);
		endResetModel();
	}

	void ProfileParameterModel::setMachineCategory(const QString& category, const QString& subCategory)
	{
		if (!m_settings)
			return;
		beginResetModel();
		m_filterSettings = m_settings->machineParameters(category, subCategory);
		endResetModel();
	}

	int ProfileParameterModel::rowCount(const QModelIndex& parent) const
	{
		return m_filterSettings.count();
	}

	QVariant ProfileParameterModel::data(const QModelIndex& index, int role) const
	{
		int i = index.row();
		if (i < 0 || i >= rowCount())
			return QVariant(QVariant::Invalid);

		us::USetting* setting = m_filterSettings.at(i);
		us::SettingItemDef* def = setting->def();
		if (role == NameRole)
			return QVariant(def->name);
		if (role == NameRole1)
			return QVariant(def->type);
		if (role == NameRole2)
		{
			return QVariant(value(setting->key()));
		}

		if (role == NameRole3)
		{
			QJSValue value = invokeJS(def->enableStatus, const_cast<ProfileParameterModel*>(this));
			bool enabled = value.toBool();
			return QVariant(enabled);
		}

		if (role == NameRole4)
			return QVariant(QVariant::Invalid);
		if (role == NameRole5)
		{
			
			ParameterBase* base = qobject_cast<ParameterBase*>(this->parent());
			us::USettings* userSettings = base->userSettings();
			us::USetting* usetting = qobject_cast<us::USetting*>(userSettings->settingObject(setting->key()));
			if (usetting)
			{
				return QVariant(usetting->enumIndex());
			}
			return QVariant(setting->enumIndex());
		}
			
		if (role == NameRole6)
			return QVariant(def->unit);
		if (role == NameRole7)
			return QVariant(def->description);
		if (role == NameRole8)
			return QVariant(def->valueStr);
		if (role == NameRole9)
			return QVariant(def->label.trimmed());
		if (role == NameRole10)
		{
			return QVariant(isUserSetting(def->name));
		}
		if (role == NameRole11)
		{
			bool enabled = value(setting->key()) == "true" ? true : false;
			return QVariant(enabled);
		}
		if (role == NameRole12)
		{
			if (def->items.size() > 0 && def->type == "bool")
			{
				return 1;
			}
			return def->level;
		}
		if (role == NameRole13)
		{//材料参数配置页面，决定前面复选框是否被选中的值
			QStringList keys = m_userSettings->settings().keys();
			return keys.contains(setting->key());
		}

		return QVariant(QVariant::Invalid);
	}
	void ProfileParameterModel::resetValue(QString key)
	{
		ParameterBase* base = qobject_cast<ParameterBase*>(this->parent());
		if (base)
		{
			//
			us::USettings* userSettings = base->userSettings();
			us::USetting* setting = qobject_cast<us::USetting*>(m_settings->settingObject(key));
			if (!setting)
			{
				return;
			}
			else {
				userSettings->deleteValueByKey(key);
				base->save();
				int row = m_filterSettings.indexOf(setting);
				if (row >= 0)
				{
					emit dataChanged(createIndex(row, 0), createIndex(row, 0));
				}
				reCalculateSetting(key);
				m_affectedKeys.remove(key);
				base->setDirty();
				return;
			}
		}
	}

	void ProfileParameterModel::setOverideValue(QString key, QString value)
	{
		ParameterBase* base = qobject_cast<ParameterBase*>(this->parent());
		if (base)
		{
			us::USettings* userSettings = base->userSettings();
			us::USetting* usetting = userSettings ? qobject_cast<us::USetting*>(userSettings->settingObject(key)) : nullptr;
			if (usetting)
			{
				us::USetting* setting = qobject_cast<us::USetting*>(m_settings->settingObject(key));

				if (setting != nullptr && fixFloatValue(setting->str(), setting) == value)
				{
					resetValue(key);
					base->setDirty();
					return;
				}
				else {
					usetting->setValue(value);
					base->save();
				}
			}
			else {
				userSettings->add(key, value);
				usetting = qobject_cast<us::USetting*>(userSettings->settingObject(key));
				usetting->setValue(value);
				base->save();
			}
		}
	}

	void ProfileParameterModel::setValue(QString key, QString value)
	{
		qDebug() << "set key:" << value;
		ParameterBase* base = qobject_cast<ParameterBase*>(this->parent());
		if (base)
		{
			us::USettings* userSettings = base->userSettings();
			us::USetting* usetting = userSettings? qobject_cast<us::USetting*>(userSettings->settingObject(key)) : nullptr;
			if (usetting)
			{
				us::USetting* setting = qobject_cast<us::USetting*>(m_settings->settingObject(key));
				
				if (setting!=nullptr && fixFloatValue(setting->str(),setting) == value)
				{
					resetValue(key);
					base->setDirty();
					return;
				}
				else {
					usetting->setValue(value);
					base->save();
				}
			}
			else {
				us::USetting* setting = qobject_cast<us::USetting*>(m_settings->settingObject(key));
				if (setting)
				{
					if (fixFloatValue(setting->str(), setting) == value)
					{
						qDebug() << "set key2:" << value;
						base->setDirty();
						return;
					}
				}
				userSettings->add(key, value);
				usetting = qobject_cast<us::USetting*>(userSettings->settingObject(key));
				usetting->setValue(value);
				base->save();
				int row = m_filterSettings.indexOf(setting); 
				if (row >= 0)
				{
					emit dataChanged(createIndex(row, 0), createIndex(row, 0));
					Q_FOREACH(us::SettingItemDef* def,usetting->def()->items)
					{
						us::USetting*  child_setting = qobject_cast<us::USetting*>(m_settings->settingObject(def->name));
						if (child_setting)
						{
							row = m_filterSettings.indexOf(child_setting);
							if (row >= 0)
							{
								emit dataChanged(createIndex(row, 0), createIndex(row, 0));
							}
						}
					}
				}
			}
			reCalculateSetting(key);

			base->setDirty();
		}
	}
	QString ProfileParameterModel::fixFloatValue(QString value, us::USetting* setting) const
	{
		if (setting->def()->type == "float")
		{
			float fval = value.toFloat();
			return QString::number(fval, 'f', 2);
		}
		return value;
	}
	QString ProfileParameterModel::value(QString key) const
	{
		QString retVal = "";
		ParameterBase* base = qobject_cast<ParameterBase*>(this->parent());
		if (base)
		{
			//�û��޸�
			us::USettings* userSettings = base->userSettings();
			if (userSettings)
			{
				retVal = userSettings->value(key, "");
				if (retVal != "")
				{
					return retVal;
				}
			}
			//Profile����
			retVal = m_settings->value(key, "");
			if (retVal != "")
			{
				return fixFloatValue(retVal, qobject_cast<us::USetting*>(m_settings->settingObject(key)));
			}
			//��
			//us::USettings* baseSettings = base->settings();
			//if (retVal != "")
			//{
			//	return retVal;
			//}
			//��������
			PrintMachine* machine = qobject_cast<PrintMachine*>(base);
			if (!machine)
			{
				machine = qobject_cast<PrintMachine*>(base->parent());
			}
			us::USettings* umachineSetting = machine->settings();
			retVal = umachineSetting->value(key, "");
			if (retVal != "")
			{
				return fixFloatValue(retVal, qobject_cast<us::USetting*>(umachineSetting->settingObject(key)));
			}
			us::USettings* machineSetting = machine->settings();
			retVal = machineSetting->value(key, "");
			if (retVal != "")
			{
				return fixFloatValue(retVal, qobject_cast<us::USetting*>(machineSetting->settingObject(key)));
			}
			//��ǰ����������
			PrintExtruder* extruder = qobject_cast<PrintExtruder*>(machine->extruderObject(m_extruderindex));
			if (extruder)
			{
				us::USettings* uextruderSetting = extruder->userSettings();
				if(uextruderSetting)
				{
					retVal = uextruderSetting->value(key, "");
					if (retVal != "")
					{
						return fixFloatValue(retVal, qobject_cast<us::USetting*>(uextruderSetting->settingObject(key)));
					}
				}
				
				us::USettings* extruderSetting = extruder->settings();
				if (extruderSetting)
				{
					retVal = machineSetting->value(key, "");
					if (retVal != "")
					{
						return fixFloatValue(retVal, qobject_cast<us::USetting*>(machineSetting->settingObject(key)));
					}
				}
				
			}
			retVal = m_global_settings->value(key, "");
			if (retVal != "")
			{
				return fixFloatValue(retVal, qobject_cast<us::USetting*>(m_global_settings->settingObject(key)));
			}
			qDebug() << key << " value no find";
			return retVal;
		}
		return "";
	}
	bool ProfileParameterModel::isUserSetting(QString key) const
	{
		ParameterBase* base = qobject_cast<ParameterBase*>(this->parent());
		if(base)
		{
			us::USettings* userSettings = base->userSettings();
			QString retVal = userSettings ? userSettings->value(key, "") : "";
			if (retVal != "")
			{
				return true;
			}
		}
		return false;
	}
	void ProfileParameterModel::reCalculateSetting(QString key)
	{
		
		QHash<QString, us::USetting* >::const_iterator it = m_global_settings->settings().constBegin();
		while (it != m_global_settings->settings().constEnd())
		{
			us::SettingItemDef* def = it.value()->def();
			if (!isUserSetting(def->name))
			{
				int vindex = def->valueStr.indexOf(key);
				bool bValFind = false;
				if (vindex >=0)
				{
					bValFind = true;
					if (vindex > 0)
					{
						QChar c = def->valueStr.at(vindex -1);
						if (c == "_")
						{
							bValFind = false;
						}
					}
					if(def->valueStr.size()> vindex + key.size())
					{
						QChar c = def->valueStr.at(vindex + key.size());
						if (c == "_")
						{
							bValFind = false;
						}
					}
				}
				
				if (bValFind)
				{
					if (!m_affectedKeys.contains(key))
					{
						m_affectedKeys[key] = {};
					}
					QJSValue value = invokeJS(def->valueStr,this);
					m_affectedKeys[key].push_back(def->name);
					us::USetting* setting = nullptr;
					if (m_settings->hasKey(def->name))
					{
						setting = qobject_cast<us::USetting*>(m_settings->settingObject(def->name));
					}
					else
					{
						setting = qobject_cast<us::USetting*>(m_global_settings->settingObject(def->name));
					}
					if (setting)
					{
						if (setting->def()->type == "float")
						{
							float fval = value.toString().toFloat();
							setting->setValue(QString::number(fval, 'f', 2));
						}
						else {
							setting->setValue(value.toString());
						}
						reCalculateSetting(def->name);
						int row = m_filterSettings.indexOf(setting);
						if (row >= 0)
						{
							emit dataChanged(createIndex(row, 0), createIndex(row, 0));
						}
					}

					//qDebug() << value.toString();
				}
				if (def->enableStatus.indexOf(key) >= 0)
				{
					us::USetting* setting = qobject_cast<us::USetting*>(m_settings->settingObject(def->name));
					if (setting)
					{
						int row = m_filterSettings.indexOf(setting);
						if (row >= 0)
						{
							emit dataChanged(createIndex(row, 0), createIndex(row, 0));
						}
					}
				}

				
			}
			it++;
		}
		
		//beginResetModel();
		//endResetModel();
	}

	bool ProfileParameterModel::setData(const QModelIndex& index, const QVariant& value, int role)
	{
		int i = index.row();
		if (i < 0 || i >= rowCount())
			return false;

		return false;
	}

	QHash<int, QByteArray> ProfileParameterModel::roleNames() const
	{
		QHash<int, QByteArray> roles;
		roles[NameRole] = "key";
		roles[NameRole1] = "type";
		roles[NameRole2] = "str";
		roles[NameRole3] = "enabled";
		roles[NameRole4] = "enums";
		roles[NameRole5] = "currentIndex";
		roles[NameRole6] = "unit";
		roles[NameRole7] = "tips";
		roles[NameRole8] = "value";
		roles[NameRole9] = "label";
		roles[NameRole10] = "userModify";
		roles[NameRole11] = "checked";
		roles[NameRole12] = "level";
		roles[NameRole13] = "isOverride";

		return roles;
	}
	void ProfileParameterModel::applyUserSetting(us::USettings* settings)
	{
		QHash<QString, us::USetting* >::const_iterator it = settings->settings().constBegin();
		while (it != settings->settings().constEnd())
		{
			us::SettingItemDef* def = it.value()->def();
			reCalculateSetting(def->name);
			it++;
		}
	}

	void creative_kernel::ProfileParameterModel::mergeMachineSettings(const int& extruderIndex)
	{
		PrintMachine* machine = qobject_cast<PrintMachine*>(this->parent());
		if (!machine)
		{
			machine = qobject_cast<PrintMachine*>(this->parent()->parent());
		}
		if (machine)
		{
			PrintExtruder* extruder = qobject_cast<PrintExtruder*>(machine->extruderObject(extruderIndex));
			ProfileParameterModel* model = qobject_cast<ProfileParameterModel*>(extruder->extruderParameterModel("line", true));
			model->mergeFilterSettings(m_settings);
			model = qobject_cast<ProfileParameterModel*>(extruder->extruderParameterModel("retraction", true));
			model->mergeFilterSettings(m_settings);
		}
	}
	void creative_kernel::ProfileParameterModel::mergeMaterialSettings(const int& extruderIndex)
	{
		PrintMachine* machine = qobject_cast<PrintMachine*>(this->parent());
		if (!machine)
		{
			machine = qobject_cast<PrintMachine*>(this->parent()->parent());
		}
		if (machine)
		{
			PrintExtruder* extruder = qobject_cast<PrintExtruder*>(machine->extruderObject(extruderIndex));
			if (!extruder)
				return;
			PrintMaterial* extruderMaterial = qobject_cast<PrintMaterial*>(machine->materialObject(extruder->materialName()));
			ProfileParameterModel* model = qobject_cast<ProfileParameterModel*>(extruderMaterial->materialParameterModel(extruderIndex,true));
			if (!model)
			{
				return;
			}
			model->setMaterialCategory("temperature");
			model->mergeFilterSettings(m_settings);
			model->setMaterialCategory("flow");
			model->mergeFilterSettings(m_settings);
			model->setMaterialCategory("cool");
			model->mergeFilterSettings(m_settings);
			auto profileName = machine->_getCurrentProfile();
			us::USettings* overrideSettings = extruderMaterial->getOverrideSettings(profileName);
			if (overrideSettings)
			{
				m_settings->merge(overrideSettings);
			}
		}
	}
	void creative_kernel::ProfileParameterModel::mergeFilterSettings(us::USettings* settings)
	{
		if (settings == nullptr)
			return;
		for (const auto& filter : m_filterSettings)
		{
			QString key = filter->key();
			if (m_userSettings && m_userSettings->hasKey(key))
			{
				if (settings->hasKey(key))
				{
					settings->findSetting(key)->setValue(m_userSettings->value(key,""));
				}
				else
				{
					settings->insert(m_userSettings->findSetting(key)->clone());
				}
			}
			else
			{
				if (settings->hasKey(key))
				{
					settings->findSetting(key)->setValue(filter->str());
				}
				else
				{
					settings->insert(filter->clone());
				}
			}
		}
	}
	bool creative_kernel::ProfileParameterModel::hasKeyInFilter(const QString& key)
	{
		for (auto settings : m_filterSettings)
		{
			if (settings->key() == key)
			{
				return true;
			}
		}
		return false;
	}
}