
#include "printmachine.h"

#include "internal/parameter/printextruder.h"
#include "internal/parameter/printprofile.h"
#include "internal/parameter/parametercache.h"
#include "internal/parameter/printmaterial.h"
#include "internal/models/printprofilelistmodel.h"
#include "internal/models/profileparametermodel.h"
#include "interface/commandinterface.h"


#include "interface/machineinterface.h"
#include "kernel/kernelui.h"
#include "cxkernel/utils/traitsutil.h"

#include <QtCore/QUrl>
#include <QDir>
#include <QtCore/QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QCoreApplication>
#include "interface/modelinterface.h"
#include "interface/spaceinterface.h"
#include "qtusercore/string/resourcesfinder.h"
#include "external/kernel/kernel.h"
#include "materialcenter.h"

namespace creative_kernel
{
	PrintMachine::PrintMachine(QObject* parent, const MachineMeta& meta, const MachineData& data, PrintType pt)
		: ParameterBase(parent), m_Pt(pt), m_usNeed(new us::USettings())
		, m_meta(meta)
        , m_data(data)
        , m_currentProfile(nullptr)
        , m_profilesModel(nullptr)
	{
        m_emptyExtruder = new PrintExtruder("", -1, this);
        m_emptyProfile = new PrintProfile("", "", this);
        m_emptyMaterial = new PrintMaterial("", MaterialData(), this);

        m_PrintMaterialModel = new PrintMaterialModel();
        if (!m_ModelProxy)
        {
            m_ModelProxy = new PrintMaterialModelProxy();
        }

        //读取机型下面用户添加的材料
        m_UserMaterialsData.clear();
        loadMaterialMeta(m_UserMaterialsData, true, uniqueName());

        QStringList materialsName;
        _getSupportMaterials(materialsName);
        QStringList mn = selectTypes();
        m_ModelProxy->setSelectMaterialTypes(getKernel()->materialCenter()->types());
        m_ModelProxy->setSourceModel(m_PrintMaterialModel);
	}

	PrintMachine::~PrintMachine()
	{

	}

	QString PrintMachine::name()
	{
        QString nozzles = "";
        for (int i = 0; i < m_meta.extruderCount; ++i)
        {
            if (i == 0)
            {
                nozzles += QString("_%1").arg(m_data.extruderDiameters.at(i));
            }
            else if (i == 1)
            {
                nozzles += QString("+%1").arg(m_data.extruderDiameters.at(i));
            }
        }
        return m_meta.baseName + nozzles + " nozzle";
	}

    QString PrintMachine::baseName()
    {
        return m_meta.baseName;
    }

    QString PrintMachine::uniqueName()
    {
        QString nozzles = "";
        for (int i = 0; i < m_meta.extruderCount; ++i)
        {
            if (i != 0)
                nozzles += "-";
            nozzles += QString("%1").arg(m_data.extruderDiameters.at(i));
        }
        return QString("%1_%2").arg(m_meta.baseName).arg(nozzles);
    }

    QString PrintMachine::uniqueBasicName()
    {
        QString nozzles = "";
        for (int i = 0; i < m_meta.extruderCount; ++i)
        {
            if (i != 0)
                nozzles += "-";
            nozzles += QString("%1").arg(m_data.extruderDiameters.at(i));
        }
        return QString("%1_%2").arg(m_meta.basicMachineName).arg(nozzles);
    }

    bool PrintMachine::isBelt()
    {
		if (m_user_settings->hasKey("machine_is_belt"))
		{
			return m_user_settings->vvalue("machine_is_belt").toBool();
		}
        return m_settings->vvalue("machine_is_belt").toBool();
    }

    bool PrintMachine::isUserMachine()
    {
        return m_meta.isUser;
    }

    bool PrintMachine::isFromUserMachine()
    {
        return m_meta.baseName == m_meta.basicMachineName;
    }

    bool PrintMachine::centerIsZero()
    {
		if (m_user_settings->hasKey("machine_center_is_zero"))
		{
			return m_user_settings->vvalue("machine_center_is_zero").toBool();
		}
		return m_settings->vvalue("machine_center_is_zero").toBool();
    }

    QString PrintMachine::fromUserMachineName()
    {
        return m_meta.basicMachineName;
    }

    float PrintMachine::machineWidth()
    {
        if (m_user_settings->hasKey("machine_width"))
        {
            return m_user_settings->vvalue("machine_width").toFloat();
        }
        return m_settings->vvalue("machine_width").toFloat();
    }
    float PrintMachine::machineDepth()
    {
        if (m_user_settings->hasKey("machine_depth"))
        {
            return m_user_settings->vvalue("machine_depth").toFloat();
        }
        return m_settings->vvalue("machine_depth").toFloat();
    }
    float PrintMachine::machineHeight()
    {
        if (m_user_settings->hasKey("machine_height"))
        {
            return m_user_settings->vvalue("machine_height").toFloat();
        }
        return m_settings->vvalue("machine_height").toFloat();
    }

    int PrintMachine::machineType()
    {
        int type = 1;
        QString machine_support_slicer_type = m_settings->vvalue("machine_support_slicer_type", "").toString();
        if (machine_support_slicer_type == "FDM-LASER-CNC")
            type = 3;
        else if (machine_support_slicer_type == "FDM-LASER")
            type = 2;

        return type;
    }

    float PrintMachine::nozzleSize(int index)
    {
        return 0.0f;
    }

    void PrintMachine::added(bool isInitialize)
    {
        setSettings(createMachineSettings(uniqueName()));
        setUserSettings(createUserMachineSettings(uniqueName()));
        m_settings->add("machine_nozzle_size", QString("%1").arg(m_data.extruderDiameters.at(0)));

        //profiles
        QStringList fileNames;
        if (m_meta.isUser)
        {//用户自定义机型
            if (isInitialize)
            {//软件启动的时候只获取路径
                fileNames = fetchFileNames(QString("%1/%2").arg(_pfpt_profiles).arg(uniqueName()));
            }
            else {
                fileNames = fetchUserProfileNames(uniqueBasicName(), uniqueName());
            }
        }
        else {//默认机型
            fileNames = fetchUserProfileNames(uniqueName());
        }

        std::sort(fileNames.begin(), fileNames.end(), [this](const QString& s1, const QString& s2) {
            int level1 = _caculateLevel(s1);
            int level2 = _caculateLevel(s2);
            return level1 < level2;
            });

        int count = m_settings->vvalue("machine_extruder_count").toInt();
        for (int i = 0; i < count; ++i)
        {
            PrintExtruder* extruder = new PrintExtruder(uniqueName(), i, this);
            extruder->added();
            m_extruders.append(extruder);

            const QList<MaterialMeta>& metas = materialMetasInMachine();
             //materials
            QStringList selectMaterialNames;
            _getSelectMaterials(selectMaterialNames);
           
            qDebug() << QString("PrintMachine::added ");
            qDebug() << selectMaterialNames;
            qDebug() << uniqueName();
            qDebug() << m_meta.supportMaterialDiameters;
            qDebug() << m_meta.supportMaterialTypes;

            m_materials.clear();
            for (const QString& selectMaterialName : selectMaterialNames)
            {
                int index = selectMaterialName.lastIndexOf("_");
                QString selectMaterial = selectMaterialName.mid(0, index);
                float selectDiameter = selectMaterialName.mid(index + 1).toFloat();
                for (const MaterialMeta& meta : metas)
                {
                    if (selectMaterial == meta.name && meta.supportDiameters.contains(selectDiameter) || selectMaterialName == meta.name)
                    {
                        for (float diameter : meta.supportDiameters)
                        {
                            if (m_meta.supportMaterialDiameters.contains(diameter))
                            {
                                MaterialData data;
                                data.brand = meta.brand;
                                data.type = meta.type;
                                data.name = meta.name;
                                data.diameter = diameter;
                                data.isUserDef = meta.isUserDef;
                                PrintMaterial* material = new PrintMaterial(uniqueName(), data, this);
                                material->added();
                                m_materials.append(material);
                                //if (isInitialize)
                                {
                                    if (i == 0)
                                    {
                                        m_selectMaterials_0.append(data.uniqueName());
                                    }
                                    else if (i == 1)
                                    {
                                        m_selectMaterials_1.append(data.uniqueName());
                                    }
                                }
                                emit materialsNameChanged();
                                break;
                            }
                        }
                    }
                }
            }
            if (m_materials.isEmpty())
            {
                continue;
            }
            //if (isInitialize)
            {
                filterMeterialsFromTypes();
            }
            //if (!isInitialize)
            {
                _cacheSelectMaterials(i);
            }
            sortMaterials();

            QStringList names = materialsNameList();
            QString materialName = readCacheExtruderMaterialIndex(uniqueName(), i);
            if (!names.contains(materialName))
            {
                if (m_meta.supportMaterialNames.length() > 0)
                {

                    Q_FOREACH(QString name, names)
                    {
                        if (name.split("_")[0]==m_meta.supportMaterialNames[0])
                        {
                            materialName = name;
                            break;
                        }
                    }
                }
            }
            extruder->setMaterial(materialName);
        }

        for (const QString& fileName : fileNames)
        {
            PrintProfile* profile = new PrintProfile(uniqueName(), fileName, this);
            profile->added();
            m_profiles.append(profile);
        }

        if (m_profiles.count() > 0)
        {
            QString profileName = _getCurrentProfile();
        }

        for (const auto& profile : m_profiles)
        {
            profile->profileParameterModel(extruderCount() == 1);
        }

        //_cacheExtruderMaterialIndex();
    }

    void PrintMachine::addImport()
    {
        //setSettings(createMachineSettings(uniqueName()));
        setSettings(createMachineSettingsImport(uniqueName()));
        setUserSettings(createUserMachineSettings(uniqueName()));
        m_settings->add("machine_nozzle_size", QString("%1").arg(m_data.extruderDiameters.at(0)));

        //profiles
        QStringList fileNames;
        if (m_meta.isUser)
        {//用户自定义机型
            fileNames = fetchUserProfileNames(uniqueBasicName(), uniqueName());
        }
        else {//默认机型
            fileNames = fetchUserProfileNames(uniqueName());
        }

        std::sort(fileNames.begin(), fileNames.end(), [this](const QString& s1, const QString& s2) {
            int level1 = _caculateLevel(s1);
            int level2 = _caculateLevel(s2);
            return level1 < level2;
            });


        int count = m_settings->vvalue("machine_extruder_count").toInt();
        for (int i = 0; i < count; ++i)
        {
            PrintExtruder* extruder = new PrintExtruder(uniqueName(), i, this);
            extruder->added();
            m_extruders.append(extruder);

            //materials
            QStringList selectMaterialNames = readCacheSelectMaterials(uniqueName(), i);

            if (selectMaterialNames.isEmpty())
            {
                selectMaterialNames = m_meta.supportMaterialNames;
            }

            const QList<MaterialMeta>& metas = materialMetas();

            if (selectMaterialNames.isEmpty())
            {
                for (const MaterialMeta& meta : metas)
                {
                    if (m_meta.supportMaterialTypes.contains(meta.type))
                    {
                        selectMaterialNames.push_back(meta.name);
                    }
                }
            }

            qDebug() << QString("PrintMachine::added ");
            qDebug() << selectMaterialNames;
            qDebug() << uniqueName();
            qDebug() << m_meta.supportMaterialDiameters;
            qDebug() << m_meta.supportMaterialTypes;

            m_materials.clear();
            for (const QString& selectMaterialName : selectMaterialNames)
            {
                for (const MaterialMeta& meta : metas)
                {
                    if (selectMaterialName == meta.uniqueName() || selectMaterialName == meta.name)
                    {
                        for (float diameter : meta.supportDiameters)
                        {
                            if (m_meta.supportMaterialDiameters.contains(diameter))
                            {
                                MaterialData data;
                                data.brand = meta.brand;
                                data.type = meta.type;
                                data.name = meta.name;
                                data.diameter = diameter;
                                data.isUserDef = meta.isUserDef;
                                PrintMaterial* material = new PrintMaterial(uniqueName(), data, this);
                                material->added();
                                m_materials.append(material);
                                emit materialsNameChanged();
                                break;
                            }
                        }
                    }
                }
            }
            if (m_materials.isEmpty())
            {
                continue;
            }
            
            _cacheSelectMaterials(i);

            QStringList names = materialsNameList();
            QString materialName = readCacheExtruderMaterialIndex(uniqueName(), i);
            if (!names.contains(materialName))
                materialName = names.at(0);

            extruder->setMaterial(materialName);
        }
        sortMaterials();
        for (const QString& fileName : fileNames)
        {
            PrintProfile* profile = new PrintProfile(uniqueName(), fileName, this);
            profile->added();
            m_profiles.append(profile);
        }

        if (m_profiles.count() > 0)
        {
            QString profileName = _getCurrentProfile();
        }
        for (const auto& profile : m_profiles)
        {
            profile->profileParameterModel(extruderCount() == 1);
        }

        //_cacheExtruderMaterialIndex();
    }

    void PrintMachine::reset()
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
            for (auto data : m_machineParameterModels)
            {
                data.first;
                data.second->resetValue(keys[i]);
            }
        }

        save();
    }

    void PrintMachine::removed()
    {
        removeUserMachineFile(uniqueName());
        removeMachineMetaUser(uniqueName());
        for (PrintExtruder* extruder : m_extruders)
            extruder->removed();
        
        for (PrintMaterial* pm : m_materials)
            pm->removed();

        for (PrintProfile* profile : m_profiles)
            profile->removed();
    }

    QAbstractListModel* PrintMachine::machineParameterModel(const QString& category, const QString& subCategory)
    {
        QString key = subCategory.isEmpty() ? category : category + "_" + subCategory;
        if (m_machineParameterModels.find(category) == m_machineParameterModels.end()) {
            ProfileParameterModel* model = new ProfileParameterModel(m_settings, this);
            model->setMachineCategory(category, subCategory);
            m_machineParameterModels[key] = model;
        }
        return m_machineParameterModels[key];
    }

    QList<QObject*> PrintMachine::generateParamCheck()
    {
        PrintProfile* pp = qobject_cast<PrintProfile*>(currentProfileObject());
        pp->profileParameterModel(true);
        m_usNeed->clear();
        QList<QObject*> printerObj = generatePrinterParamCheck();
        QList<QObject*> materialObj = generateMaterialParamCheck();
        m_ParamCheckList = printerObj + materialObj;

        return m_ParamCheckList;
    }

    void PrintMachine::resetProfileModify()
    {
        PrintProfile* pp = qobject_cast<PrintProfile*>(currentProfileObject());
        if (!m_usNeed || !pp)
        {
            return;
        }
        pp->reset(m_usNeed);
    }

    QAbstractListModel* PrintMachine::profilesModel()
    {
        if (!m_profilesModel)
            m_profilesModel = new PrintProfileListModel(this, this);
        return m_profilesModel;
    }

    QStringList PrintMachine::profileNames()
    {
        return cxkernel::objectNames(m_profiles);
    }

    int PrintMachine::curProfileIndex()
    {
        int index = m_profiles.indexOf(m_currentProfile);
        return index;
    }

    PrintProfile* PrintMachine::profile(int index)
    {
        if (index < 0 || index >= m_profiles.count())
            return nullptr;
         //   return m_emptyProfile;

        return m_profiles.at(index);
    }

    int PrintMachine::profilesCount()
    {
        return m_profiles.count();
    }

    QObject* PrintMachine::profileObject(int index)
    {
        return profile(index);
    }

    QObject* PrintMachine::currentProfileObject()
    {
        return m_currentProfile ? m_currentProfile : m_emptyProfile;
    }

    bool PrintMachine::checkMaterialName(const QString& oldName, const QString& newName)
    {
        if (oldName == newName)
        {
            return false;
        }

        bool hasMaterial = false;
        for (auto material : m_materials)
        {
            QString uName = material->uniqueName();

            if (uName.contains("Generic"))
            {
                uName.replace("Generic", QCoreApplication::translate("PrintMachine", "Generic"));
            }

            if (uName == newName)
            {
                hasMaterial = true;
                break;
            }
        }

        return hasMaterial;
    }

    bool PrintMachine::checkProfileName(const QString& name)
    {
        QString temp = name.trimmed();
        QStringList names = profileNames();
        return names.indexOf(temp) >= 0;
    }

    QString PrintMachine::generateNewProfileName(const QString& prefix)
    {
        QStringList names = profileNames();
        QString start = "new";
        QString name = prefix.isEmpty() ? start : prefix;
        int index = 1;
        while (checkProfileName(name))
        {
            name = QString("%1 %2").arg(start).arg(index);
            ++index;
        }
        return name;
    }

    void PrintMachine::addProfile(const QString& profileName, const QString& templateProfile)
    {
        _addProfile(profileName, templateProfile);
    }

    bool PrintMachine::isCurrentProfileDefault()
    {
        if (!m_currentProfile)
            return true;

        return m_currentProfile->isDefault();
    }

    Q_INVOKABLE void PrintMachine::exportPrinterFromQml(const QString& urlString)
    {
        QUrl url(urlString);

        if (!m_currentProfile)
        {
            qWarning() << QString("PrintMachine::exportProfileFromQml error empty.");
            return;
        }

        //export
        us::USettings* settings = m_settings->copy();
        if (m_user_settings)
            settings->merge(m_user_settings);

        settings->saveMachineAsDefault(url.toLocalFile());
        delete settings;
        //添加喷嘴的直径的参数
        for (int index = 0; index < m_extruders.count(); ++index)
        {
            us::USettings* pe0Settings = (qobject_cast<PrintExtruder*>(extruderObject(index)))->settings();
            pe0Settings = pe0Settings->copy();
            us::USettings* pe0UserSettings = (qobject_cast<PrintExtruder*>(extruderObject(index)))->userSettings();
            if(pe0UserSettings)
                pe0Settings->merge(pe0UserSettings);

            pe0Settings->saveExtruderAsDefault(url.toLocalFile(), index);

            if (pe0Settings) {
                delete pe0Settings;
                pe0Settings = nullptr;
            }
            
            //settings->merge(pe0Settings);
        }
       
        //settings->saveAsDefault(url.toLocalFile());
        //export
        //this->exportSetting(url.toLocalFile());
    }

    Q_INVOKABLE void PrintMachine::importMaterialFromQml(const QString& urlString)
    {
        QUrl url(urlString);

        QFileInfo info(url.toLocalFile());
        QString baseName = info.baseName();

        QString profileName = generateNewProfileName(baseName);

        copyFile2User(url.toLocalFile(), uniqueName(), profileName, true);
        PrintProfile* newProfile = new PrintProfile(uniqueName(), profileName, this);
        _addProfile(newProfile);
    }

    void PrintMachine::removeProfile(QObject* profileObject)
    {
        PrintProfile* profile = qobject_cast<PrintProfile*>(profileObject);
        _removeProfile(profile);
    }

    void PrintMachine::setCurrentProfile(int index)
    {
        if (index < 0 || index >= m_profiles.count())
        {
            qWarning() << QString("PrintMachine::setCurrentProfile error index [%1].").arg(index);
            return;
        }

        _setCurrentProfile(m_profiles.at(index));
    }

    void PrintMachine::exportProfileFromQml(const QString& urlString)
    {
        QUrl url(urlString);

        if (!m_currentProfile)
        {
            qWarning() << QString("PrintMachine::exportProfileFromQml error empty.");
            return;
        }

        m_currentProfile->exportSetting(url.toLocalFile());
    }

    void PrintMachine::exportProfileFromQmlIni(const QString& urlString)
    {
        QUrl url(urlString);

        if (!m_currentProfile)
        {
            qWarning() << QString("PrintMachine::exportProfileFromQml error empty.");
            return;
        }

        m_currentProfile->exportSettingIni(url.toLocalFile());
    }

    void PrintMachine::importProfileFromQml(const QString& urlString)
    {
        QUrl url(urlString);

        QFileInfo info(url.toLocalFile());
        QString baseName = info.baseName();

        QString profileName = generateNewProfileName(baseName);

        copyFile2User(url.toLocalFile(), uniqueName(), profileName, false);
        //导入的配置参数分拆出来cover文件
        //1.对比耗材和配置文件的不同
        //2.对比挤出机和配置文件的不同
        //3.写入cover文件
        PrintProfile* newProfile = new PrintProfile(uniqueName(), profileName, this);
        _addProfile(newProfile);
        newProfile->profileParameterModel(extruderCount() == 1);
    }

    void PrintMachine::importProfileFromQmlIni(const QString& urlString)
    {
        QUrl url(urlString);

        QFileInfo info(url.toLocalFile());
        QString baseName = info.baseName();

        QString profileName = generateNewProfileName(baseName);

        copyFile2User(url.toLocalFile(), uniqueName(), profileName, false);
        //导入的配置参数分拆出来cover文件
        PrintExtruder* pe = m_extruders.at(0);
        us::USettings* peSettings = pe->settings();
        us::USettings* uSettings = pe->userSettings();
        us::USettings* tempSettings = peSettings->copy();
        tempSettings->merge(uSettings);

        QSettings settingSourceIni(url.toLocalFile(), QSettings::IniFormat);
        QString dest = userProfileFile(uniqueName(), profileName, true, pe->materialName());
        QSettings settingDestIni(dest, QSettings::IniFormat);
        QStringList materialKeys = creative_kernel::loadMaterialKeys();
        QStringList extruderKeys = creative_kernel::loadExtruderKeys("");

        PrintMaterial* pm = m_materials.at(0);
        us::USettings* pmSettings = pm->settings();
        us::USettings* pmUSettings = pm->userSettings();
        us::USettings* pmTempSettings = pmSettings->copy();
        pmTempSettings->merge(pmUSettings);
        for (QString key : extruderKeys)
        {
            QString iniValue = settingSourceIni.value(QString("/Extruder/%1").arg(key)).toString();
            QString settingValue = tempSettings->vvalue(key).toString();
            if (iniValue != settingValue && !iniValue.isEmpty() && !settingValue.isEmpty())
            {
                settingDestIni.setValue(key, iniValue);
            }
        }

        for (QString key : materialKeys)
        {
            QString iniValue = settingSourceIni.value(QString("/Material/%1").arg(key)).toString();
            QString settingValue = pmTempSettings->vvalue(key).toString();
            if (iniValue != settingValue && !iniValue.isEmpty() && !settingValue.isEmpty())
            {
                settingDestIni.setValue(key, iniValue);
            }
        }

        //1.对比耗材和配置文件的不同
        //2.对比挤出机和配置文件的不同
        //3.写入cover文件
        PrintProfile* newProfile = new PrintProfile(uniqueName(), profileName, this);
        _addProfile(newProfile);
        newProfile->profileParameterModel(extruderCount() == 1);
    }

    int PrintMachine::extruderCount()
    {
        return m_extruders.count();
    }

    bool PrintMachine::currentMachineSupportExportImage()
    {
        bool have = m_settings->vvalue("has_preview_img").toBool();
        return have;
    }

    QObject* PrintMachine::extruderObject(int index)
    {
        if (index >= 0 && index < m_extruders.count())
            return m_extruders.at(index);
        return m_emptyExtruder;
    }

    int PrintMachine::extruderMaterialIndex(int extruderIndex)
    {
        if (extruderIndex >= 0 && extruderIndex < m_extruders.count())
        {
            QString materialName = m_extruders.at(extruderIndex)->materialName();
            return materialsNameList().indexOf(materialName);
        }
        return -1;
    }

    QString PrintMachine::extruderMaterialName(int extruderIndex)
    {
        if (extruderIndex >= 0 && extruderIndex < m_extruders.count())
        {
            QString materialName = m_extruders.at(extruderIndex)->materialName();
            return materialName;
        }

        return QString();
    }

    void PrintMachine::setExtruderMaterial(int extruderIndex, const QString& materialName)
    {
        if (extruderIndex >= 0 && extruderIndex < m_extruders.count())
        {
            PrintExtruder* extruder = m_extruders.at(extruderIndex);
            QStringList names = materialsNameList();
            //names.append(userMaterialsName());
            if (names.contains(materialName))
            {
                writeCacheExtruderMaterialIndex(currentMachineName(), extruderIndex, materialName);
                extruder->setMaterial(materialName);
                extruder->setDirty();
                for (const auto& profile : m_profiles)
                {
                    profile->profileParameterModel(extruderCount() == 1);
                }
                return;
            }

            qDebug() << QString("PrintMachine::setExtruderMaterial invalid material index [%1]").arg(materialName);
            return;
        }

        qDebug() << QString("PrintMachine::setExtruderMaterial invalid extruder index [%1]").arg(extruderIndex);
    }

    void PrintMachine::setExtruderMaterial(int extruderIndex, int materialIndex)
    {
        QString materialName = materialsNameList().at(materialIndex);
        if (extruderIndex >= 0 && extruderIndex < m_extruders.count())
        {
            PrintExtruder* extruder = m_extruders.at(extruderIndex);
            QStringList names = materialsNameList();
            //names.append(userMaterialsName());
            if (names.contains(materialName))
            {
                writeCacheExtruderMaterialIndex(currentMachineName(), extruderIndex, materialName);
                extruder->setMaterial(materialName);
                extruder->setDirty();
                for (const auto& profile : m_profiles)
                {
                    profile->profileParameterModel(extruderCount() == 1);
                }
                return;
            }

            qDebug() << QString("PrintMachine::setExtruderMaterial invalid material index [%1]").arg(materialName);
            return;
        }

        qDebug() << QString("PrintMachine::setExtruderMaterial invalid extruder index [%1]").arg(extruderIndex);
    }

    bool PrintMachine::modifyMaterialName(const QString& oldMaterialName, const QString& newMaterialName)
    {
        if (oldMaterialName == newMaterialName)
        {
            return false;
        }
        //1.修改左侧列表的材料名称
        QString curMaterialUName;
        QString oldMaterialUName;
        bool hasOldMaterial = false;
        for (PrintMaterial* mm : m_materials)
        {
            if (mm->name() == oldMaterialName)
            {
                hasOldMaterial = true;
                oldMaterialUName = mm->uniqueName();
                mm->setName(newMaterialName);
                curMaterialUName = mm->uniqueName();
            }
        }
        if (!hasOldMaterial)
            return false;
        emit materialsNameChanged();

        //2.修改本地材料文件的文件名称
        QString defFileName = userMaterialFile(uniqueName(), oldMaterialUName, false);
        QFile mf(defFileName);
        if (mf.exists())
        {
            if (mf.isOpen())
            {
                mf.close();
            }
            QString newFilePath = userMaterialFile(uniqueName(), curMaterialUName, false);
            bool res = mf.rename(newFilePath);
            int a = 0;
        }

        //3.修改material_user.json里的材料名称
        reNameUserMaterialFromJson(oldMaterialUName, newMaterialName, uniqueName());

        //4.修改注册表里的材料名称
        reNameMachineMaterial(uniqueName(), oldMaterialUName, curMaterialUName, 0);

        //5.修改机器保存的材料对象列表的材料名称
        
        //6.刷新修改的内容
        return true;
    }
	
    QStringList PrintMachine::supportTypes()
    {
        return m_meta.supportMaterialTypes;
    }

    QStringList PrintMachine::selectTypes(int extruderIndex)
    {
        QStringList selectMaterialNames;
        _getSupportMaterials(selectMaterialNames);/*readCacheSelectMaterials(uniqueName(), extruderIndex);*/

        if (selectMaterialNames.isEmpty())
        {
            for (QString mName : m_meta.supportMaterialNames)
            {
                QString temp = QString("%1_%2").arg(mName).arg(m_meta.supportMaterialDiameters.at(0));
                selectMaterialNames.append(temp);
            }
        }

        const QList<MaterialMeta>& metas = materialMetasInMachine();

        QMultiMap<QString, QString> materialMap;
        for (QString mName : selectMaterialNames)
        {
            for (MaterialMeta mm : metas)
            {
                if (mm.uniqueName() == mName)
                {
                    materialMap.insert(mm.type, mName);
                }
            }
        }

        return materialMap.uniqueKeys();
    }

    void PrintMachine::selectChanged(bool checked, QString materialName, int extruderIndex)
    {
        for (auto data : m_SupportMeterialsDataList)
        {
            if (data->name == materialName)
            {
                data->isChecked = checked;
            }
        }

        if (extruderIndex == 0)
        {
            if (!checked)
            {
                m_selectMaterials_0.removeOne(materialName);
            }
            else {
                m_selectMaterials_0.append(materialName);
            }
        }
        else {
            if (!checked)
            {
                m_selectMaterials_1.removeOne(materialName);
            }
            else {
                m_selectMaterials_1.append(materialName);
            }
        }
    }

    QObject* PrintMachine::materialObject(int index)
    {
        if (index >= 0 && index < m_materials.count())
            return m_materials.at(index);
        return m_emptyMaterial;
    }

    QObject* PrintMachine::materialObject(const QString& materialName)
    {
        for (PrintMaterial* data : m_materials)
        {
            if (data->uniqueName() == materialName)
            {
                return data;
            }
        }
        return m_emptyMaterial;
    }

    void PrintMachine::filterMeterialsFromTypes()
    {
        m_SupportMeterialsDataList.clear();
        QStringList curgroupFilter;
        const QList<MaterialMeta>& allMetas = materialMetas();
        QStringList supportMaterials;
        //_getSupportMaterials(supportMaterials);
        m_PrintMaterialModel->setSupportMaterialNames(getKernel()->materialCenter()->types());
        //foreach(auto filterType, m_meta.supportMaterialTypes) 
        //{
           // qDebug() << "type =" << filterType;
            for (int i = 0; i < allMetas.size(); i++)
            {
                QString mType = allMetas.at(i).type.trimmed();
                    MaterialMeta* data = new MaterialMeta;
                    data->name = allMetas.at(i).name;
                    data->supportDiameters = allMetas.at(i).supportDiameters;
                   
                    if (m_selectMaterials_0.indexOf(data->uniqueName()) >= 0)
                    {
                        data->isChecked = true;
                    }

                    data->type = mType;
                    m_SupportMeterialsDataList.append(data);
                    m_PrintMaterialModel->addMaterial(data);

            }
        //}
    }

    void PrintMachine::setExtruderStatus(bool bextruder0, bool bextruder1)
    {
        int enableCount = (bextruder0 ? 1 : 0) + (bextruder1 ? 1 : 0);
        if (extruderCount() > 1)
        {
            if (this->m_currentProfile)
            {
                ProfileParameterModel* model = qobject_cast<ProfileParameterModel*>(this->m_currentProfile->profileParameterModel(false));
                if (model)
                {
                    model->setValue("extruders_enabled_count", QString::number(enableCount));
                }
            }
            PrintExtruder* extruder0 = m_extruders.at(0);
            extruder0->setEnabled(extruder0);
            PrintExtruder* extruder1 = m_extruders.at(1);
            extruder1->setEnabled(extruder1);
            if (bextruder0 && !bextruder1)
            {
                QList<ModelN*> selections = modelns();
                setModelsNozzle(selections, 0);
            }
            if (bextruder1 && !bextruder0)
            {
                QList<ModelN*> selections = modelns();
                setModelsNozzle(selections, 1);
            }
            if (bextruder0 && bextruder1)
            {
                //QList<ModelN*> selections = modelns();
                //setModelsNozzle(selections, 0);
            }
        }
    }

    Q_INVOKABLE QVariant creative_kernel::PrintMachine::materialsModel()
    {
        return QVariant::fromValue(m_ModelProxy);
    }

    void PrintMachine::addUserMaterial(const QString& materialModel, const QString& userMaterialName, int extruderNum)
    {
        //更新数据
        PrintMaterial* pMaterial = nullptr;
        //const QList<MaterialMeta>& metas = materialMetas();
		const QList<MaterialMeta>& metas = materialMetasInMachine();

        for (auto material : m_materials)
        {
            if (material->uniqueName() == materialModel)
            {
                pMaterial = material;
            }
        }

        for (const MaterialMeta& meta : metas)
        {
            if (materialModel.split("_")[0] == meta.name)
			//if (materialModel == meta.uniqueName())
            {
                for (float diameter : meta.supportDiameters)
                {
                    if (m_meta.supportMaterialDiameters.contains(diameter))
                    {
                        MaterialData data;
                        data.brand = meta.brand;
                        data.type = meta.type;
                        data.name = userMaterialName;
                        data.diameter = diameter;
                        data.isUserDef = true;

                        //1. 拷贝耗材文件
                        QString fileName = userMaterialFile(uniqueName(), data.uniqueName());
                        QFile file(fileName);
                        if (file.exists())
                            return;

                 /*       QString defFileName = userMaterialFile(uniqueName(), materialModel);

                        QFile defFile(defFileName);
                        if (defFile.exists())
                        {
                            qtuser_core::copyFile(defFileName, fileName, true);
                        }*/

                        pMaterial->exportSetting(fileName);

                        //2.创建材料对象
                        PrintMaterial* material = new PrintMaterial(uniqueName(), data, this);
                        material->addedUserMaterial();

                        //3.添加材料对象
                        m_materials.append(material);
                        //emit materialsNameChanged();

                        //4.添加材料到json
                        MaterialMeta metaNew = meta;
                        metaNew.name = userMaterialName;
                        if (extruderNum == 0)
                        {
                            m_selectMaterials_0.append(data.uniqueName());
                            //emit userMaterialsNameChanged();
                        }
                        else if (extruderNum == 1) 
                        {
                            m_selectMaterials_1.append(data.uniqueName());
                            //emit userMaterialsNameChanged();
                        }
                        metaNew.isUserDef = true;
                        m_UserMaterialsData.append(metaNew);
                        _addUserMaterialToJson(metaNew);

                        break;
                    }
                }
            }
        }
        sortMaterials();
        emit userMaterialsNameChanged();
        emit materialsNameChanged();
        _cacheSelectMaterials(extruderNum);
    }

    Q_INVOKABLE void creative_kernel::PrintMachine::deleteUserMaterial(const QString& materialName, int index)
    {
        //removeUserProfileFile(m_machineName, uniqueName());
        //1.获取耗材对象
        int mIndex = 0;
        for (auto materialObj : m_materials)
        {
            if (materialObj && materialObj->uniqueName() == materialName)
            {
                //1.删除耗材文件
                materialObj->removed();
                //2.删除并释放材料对象
                //materialObj->deleteLater();
                /*delete materialObj;
                materialObj = nullptr;*/
                m_materials.removeAt(mIndex);
                break;
           }           
            ++mIndex;
        }
        //3.从json删除自定义耗材
        removeUserMaterialFromJson(materialName, uniqueName());
        //4.删除配置文件
        removeMachineMaterials(uniqueName(), QStringList()<< materialName, index);
		emit materialsNameChanged();
    }

    Q_INVOKABLE void creative_kernel::PrintMachine::importMaterial(const QString& srcPath, const QString& userMaterialName, int extruderNum)
    {
        //1.判断源路径文件是否存在
        QUrl url(srcPath);
        QFile materialFile(url.toLocalFile());
        if (!materialFile.exists())
        {
            qDebug() << "文件不存在";
            return;
        }
        if (!materialFile.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            return;
        }
        //2.获取文件内部保存的材料类型
        QString materialType = "";
        while (!materialFile.atEnd())
        {
            QByteArray line = materialFile.readLine();
            QString qLine(line);
            if (qLine.contains("material_name"))
            {
                materialType = qLine.mid(QString("material_name=").length());
            }
        }
        materialFile.close();

        //3.更新数据
        const QList<MaterialMeta>& metas = materialMetas();

        for (const MaterialMeta& meta : metas)
        {
            if (materialType.split("_")[0] == meta.name)
            {
                for (float diameter : meta.supportDiameters)
                {
                    if (m_meta.supportMaterialDiameters.contains(diameter))
                    {
                        MaterialData data;
                        data.brand = meta.brand;
                        data.type = meta.type;
                        data.name = userMaterialName;
                        data.diameter = diameter;
                        data.isUserDef = true;

                        //1. 拷贝耗材文件
                        QString defFileName = userMaterialFile(uniqueName(), data.uniqueName());
                        qtuser_core::copyFile(url.toLocalFile(), defFileName, true);

                        //2.创建材料对象
                        PrintMaterial* material = new PrintMaterial(uniqueName(), data, this);
                        material->addedUserMaterial();

                        //3.添加材料对象
                        m_materials.append(material);
                        emit materialsNameChanged();

                        //4.添加材料到json
                        MaterialMeta metaNew = meta;
                        metaNew.name = userMaterialName;
                        metaNew.isUserDef = true;
                        if (extruderNum == 0)
                        {
                            m_selectMaterials_0.append(userMaterialName);
                            emit userMaterialsNameChanged();
                        }
                        else if (extruderNum == 1)
                        {
                            m_selectMaterials_1.append(userMaterialName);
                            emit userMaterialsNameChanged();
                        }
                        m_UserMaterialsData.append(metaNew);
                        _addUserMaterialToJson(metaNew);
                        break;
                    }
                }
            }
        }
        sortMaterials();
        _cacheSelectMaterials(extruderNum);
    }

    void PrintMachine::finishMeterialSelect(const int& index)
    {
        auto func = [this](MaterialMeta* mData) {
            PrintMaterial* material = nullptr;
            const QList<MaterialMeta>& metas = materialMetasInMachine();
            for (const MaterialMeta& meta : metas)
            {
                if (mData->name == meta.name)
                {
                    for (float diameter : meta.supportDiameters)
                    {
                        if (m_meta.supportMaterialDiameters.contains(diameter))
                        {
                            MaterialData data;
                            data.brand = meta.brand;
                            data.type = meta.type;
                            data.name = meta.name;
                            data.diameter = diameter;
                            data.isUserDef = meta.isUserDef;
                            material = new PrintMaterial(uniqueName(), data, this);
                           /* material->added();
                            m_materials.append(material);*/
                            return material;
                        }
                    }
                }
            }
        };

        m_materials.clear();
        m_selectMaterials_0.clear();
        foreach (auto m, m_SupportMeterialsDataList)
        {
            PrintMaterial* pMaterial = func(m);

            if (m->isChecked)
            {
                QString fileName = userMaterialFile(uniqueName(), pMaterial->uniqueName(), false);
                pMaterial->added();
                m_materials.append(pMaterial);
                m_selectMaterials_0.append(pMaterial->uniqueName());
            }
            else if(!m->isChecked)
            {
                QString fileName = userMaterialFile(uniqueName(), pMaterial->uniqueName(), false);
                QFile file(fileName);
                if (file.exists())
                {
                    file.remove();
                }
            }
        }

        //选中的材料再追加上用户自定义材料
        for (MaterialMeta um : m_UserMaterialsData)
        {
            MaterialData data;
            data.brand = um.brand;
            data.type = um.type;
            data.name = um.name;
            data.diameter = um.supportDiameters.at(0);
            data.isUserDef = um.isUserDef;
            PrintMaterial* material = new PrintMaterial(uniqueName(), data, this);
            material->addedUserMaterial();

            //3.添加材料对象
            m_materials.append(material);
            m_selectMaterials_0.append(um.uniqueName());
        }
        sortMaterials();
        emit materialsNameChanged();
        emit m_extruders[index]->materialIndexChanged();
        _cacheSelectMaterials(index);

    }

    bool PrintMachine::machineDirty()
    {
        if (dirty())
            return true;

        if (m_currentProfile && m_currentProfile->dirty())
            return true;

        bool dirty = false;
        for (PrintExtruder* extruder : m_extruders)
        {
            if (extruder->dirty())
            {
                dirty = true;
                break;
            }

            QString materialName = extruder->materialName();
            PrintMaterial* material = _findMaterial(materialName);
            if (material && material->dirty())
            {
                dirty = true;
                break;
            }
        }

        return dirty;
    }

    void PrintMachine::clearMachineDirty()
    {
        clearDiry();
        if (m_currentProfile)
            m_currentProfile->clearDiry();

        for (PrintExtruder* extruder : m_extruders)
        {
            extruder->clearDiry();
            QString materialName = extruder->materialName();
            PrintMaterial* material = _findMaterial(materialName);
            if (material)
                material->clearDiry();
        }
    }

    QSharedPointer<us::USettings> PrintMachine::createCurrentGlobalSettings()
    {
        QSharedPointer<us::USettings> globalSetting(createDefaultGlobal());
        if (m_currentProfile)
        {
            globalSetting->merge(m_currentProfile->settings());
            globalSetting->merge(m_currentProfile->userSettings());
        }
        globalSetting->merge(m_settings);
        globalSetting->merge(this->m_user_settings);
        return globalSetting;
    }

    QList<QSharedPointer<us::USettings>> PrintMachine::createCurrentExtruderSettings()
    {
        QList<QSharedPointer<us::USettings>> extruderSettings;
        for (PrintExtruder* extruder : m_extruders)
        {
            us::USettings* settings = extruder->settings();
            QSharedPointer<us::USettings> extruderSetting(settings->copy());
            QString materialName = extruder->materialName();
            PrintMaterial* material = _findMaterial(materialName);
            if (material)
            {
                extruderSetting->merge(material->settings());
                extruderSetting->merge(material->userSettings());
            }
            extruderSetting->merge(extruder->userSettings()->copy());

            if (m_extruders.size() == 1)
            {
                const auto& affectedKeys = m_currentProfile->getAffetecdKeys();
                for (const auto& profileSettings : m_currentProfile->userSettings()->settings())
                {
                    const auto& key = profileSettings->key();
                    if (extruderSetting->hasKey(key))
                    {
                        extruderSetting->findSetting(key)->setValue(m_currentProfile->getModelValue(key));
                        if (affectedKeys.contains(key))
                        {
                            for (const auto& affectedKey : affectedKeys[key])
                            {
                                QString value = m_currentProfile->getModelValue(affectedKey);
                                if (extruderSetting->hasKey(affectedKey)) 
                                {
                                    extruderSetting->findSetting(affectedKey)->setValue(value);
                                }
                                else
                                {
                                    extruderSetting->add(affectedKey, value);
                                }
                            }
                        }
                    }
                }
            }
            //if (extruder->getEnabled())
            {
                extruderSettings.append(extruderSetting);
            }   
        }

#if 0 //_DEBUG
        for (SettingsPointer settings : extruderSettings)
            settings->print();
#endif

        return extruderSettings;
    }

    void creative_kernel::PrintMachine::_addUserMaterialToJson(const MaterialMeta& materialMeta)
    {
        //需要实现
        saveMateriaMeta(materialMeta, uniqueName());
    }

    void PrintMachine::_addProfile(const QString& profileName, const QString& templateProfile)
    {
        PrintProfile* pp = qobject_cast<PrintProfile*>(currentProfileObject());
        us::USettings* us = pp->userSettings();
        us::USettings* ss = pp->settings();
        us::USettings* temp = ss->copy();
        temp->merge(us);

        QString newProfileName = userProfileFile(uniqueName(), profileName);
        temp->saveAsDefault(newProfileName);

        //拷贝cover文件
        PrintExtruder* pe = m_extruders.at(0);
        QString dst = userProfileFile(uniqueName(), profileName, true, pe->materialName());
        QString source = userProfileFile(uniqueName(), templateProfile, true, pe->materialName());
        qtuser_core::copyFile(source, dst);

        PrintProfile* newProfile = new PrintProfile(uniqueName(), profileName, this);
        _addProfile(newProfile);
    }

    void PrintMachine::_addProfile(PrintProfile* profile)
    {
        if (!profile)
            return;

        profile->added();
        m_profiles.append(profile);
        
        profilesModel();
        if (m_profilesModel)
            m_profilesModel->notifyReset();

        _setCurrentProfile(profile);
    }

    void PrintMachine::_removeProfile(PrintProfile* profile)
    {
        if (!profile)
            return;

        if (!m_profiles.contains(profile))
        {
            qWarning() << QString("PrintMachine::_removeProfile not added [%1]")
                .arg(profile->uniqueName());
            return;
        }

        profile->removed();
        m_profiles.removeOne(profile);
        profile->deleteLater();

        profilesModel();
        if (m_profilesModel)
            m_profilesModel->notifyReset();

        PrintProfile* nextProfile = nullptr;
        if (m_currentProfile == profile)
        {
            m_currentProfile = nullptr;
            if (m_profiles.count() > 0)
                nextProfile = m_profiles.at(0);
        }

        if (nextProfile)
            _setCurrentProfile(nextProfile);
    }

    void PrintMachine::_setCurrentProfile(PrintProfile* profile)
    {
        if (m_currentProfile == profile)
        {
            qDebug() << QString("PrintMachine::_setCurrentProfile same profile [%1]")
                .arg(profile ? profile->name() : QString(""));
            return;
        }

        m_currentProfile = profile;
        if (m_currentProfile)
            m_currentProfile->setDirty();

        profile->setPrintMachine(this);
        _cacheCurrentProfileIndex();

        emit curProfileIndexChanged();
        emit isCurrentProfileDefaultChanged();
    }

    void PrintMachine::_cacheCurrentProfileIndex()
    {
        int index = m_profiles.indexOf(m_currentProfile);
        //writeCacheCurrentProfile(uniqueName(), index);
        writeCacheCurrentProfile(uniqueName(), m_currentProfile->uniqueName());
    }

    void PrintMachine::_getSelectMaterials(QStringList& selectMaterialNames)
    {
        const QList<MaterialMeta>& metas = materialMetas();
        //1.读取注册表
        //selectMaterialNames = readCacheSelectMaterials(uniqueName(), 0);

        //2.读取机型支持的耗材名称
        /*
        if (selectMaterialNames.isEmpty())
        {
            for (auto mName : m_meta.supportMaterialNames)
            {
                for (const MaterialMeta& meta : metas)
                {
                    if (mName == meta.name)
                    {
                        selectMaterialNames.append(meta.uniqueName());
                    }
                }
            }
            //2.1
        }*/

        //3.读取机型文件夹下的耗材
        if (selectMaterialNames.isEmpty())
        {
            QString defFileName = QString("%1/%2").arg(_pfpt_materials)
                .arg(uniqueName());
            QDir dir(defFileName);
            QFileInfoList files = dir.entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);
            for (QFileInfo fileInfo : files)
            {
                QString suffix = fileInfo.suffix();
                if (suffix == "default")
                {
                    QString mname = fileInfo.fileName().left(fileInfo.fileName().length() - suffix.length() - 1);
                    for (auto meta : metas)
                    {
                        if (meta.name == mname.split('_').at(0))
                        {
                            selectMaterialNames.append(mname);
                            break;
                        }
                    }
                    
                }
                
            }
            if (selectMaterialNames.isEmpty())
            {
                QString defFileName = QString("%1/%2").arg(_pfpt_default_materials)
                    .arg(uniqueName2BaseMachineName(uniqueName()));
                QDir dir(defFileName);
                QFileInfoList files = dir.entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);
                for (QFileInfo fileInfo : files)
                {
                    QString suffix = fileInfo.suffix();
                    selectMaterialNames.append(fileInfo.fileName().left(fileInfo.fileName().length() - suffix.length() - 1));
                }
            }
            //selectMaterialNames = files;
        }
       


        //追加自定义耗材
        Q_FOREACH(MaterialMeta meta ,m_UserMaterialsData)
        {
            selectMaterialNames.append(meta.name);
        }

        //4.读取机型支持的耗材类型进行筛选
        /*if (selectMaterialNames.isEmpty())
        {
            for (const MaterialMeta& meta : metas)
            {
                if (m_meta.supportMaterialTypes.contains(meta.type))
                {
                    selectMaterialNames.push_back(meta.name);
                }
            }
        }*/
    }

    void PrintMachine::_getSupportMaterials(QStringList& supportMaterialNames)
    {
        const QList<MaterialMeta>& metas = materialMetasInMachine();
        //1.读取注册表(支持的耗材不能从注册表读取，那只是选中的耗材)
        //supportMaterialNames = readCacheSelectMaterials(uniqueName(), 0);

        //2.读取机型支持的耗材名称
        if (supportMaterialNames.isEmpty())
        {
            for (auto mName : m_meta.supportMaterialNames)
            {
                for (const MaterialMeta& meta : metas)
                {
                    if (mName == meta.name)
                    {
                        supportMaterialNames.append(meta.uniqueName());
                    }
                }
            }
        }

        //3.读取机型文件夹下的耗材
        if (supportMaterialNames.isEmpty())
        {
            QString defFileName = QString("%1/%2").arg(_pfpt_default_materials)
                .arg(uniqueName2BaseMachineName(uniqueName()));
            QDir dir(defFileName);
            QFileInfoList files = dir.entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);
            for (QFileInfo fileInfo : files)
            {
                QString suffix = fileInfo.suffix();
                supportMaterialNames.append(fileInfo.fileName().left(fileInfo.fileName().length() - suffix.length() - 1));
            }
            //selectMaterialNames = files;
        }

        //4.读取机型支持的耗材类型进行筛选
        if (supportMaterialNames.isEmpty())
        {
            for (const MaterialMeta& meta : metas)
            {
                if (m_meta.supportMaterialTypes.contains(meta.type))
                {
                    supportMaterialNames.push_back(meta.name);
                }
            }
        }
    }
	QList<QObject*> PrintMachine::generatePrinterParamCheck()
    {
        PrintProfile* pp = qobject_cast<PrintProfile*>(currentProfileObject());
        us::USettings* us = pp->userSettings();
        m_usNeed->clear();

        //配置文件：挤出机相关
        QList<us::USetting*> re_settings = us->extruderParameters("retraction");
        QList<us::USetting*> li_settings = us->extruderParameters("line");
        QList<us::USetting*> profile_extruder_settings = re_settings + li_settings;
         
        //打印机：挤出机相关
        PrintExtruder* pe = qobject_cast<PrintExtruder*>(extruderObject(0));
        QList<us::USetting*> printer_extruder_settings;
        QList<us::USetting*> printer_extruder_user_settings;
        if (pe)
        {
            printer_extruder_settings = pe->settings()->extruderParameters("retraction", true) + 
                pe->settings()->extruderParameters("line", true);
            printer_extruder_user_settings = pe->userSettings()->extruderParameters("retraction", true) + 
                pe->userSettings()->extruderParameters("line", true);
        }

        QList<QObject*> paramCheckList;
        for (us::USetting* setting : profile_extruder_settings)
        {
            ParamCheckStruct* pcs = new ParamCheckStruct();
            pcs->keyStr = setting->label();
            pcs->defaultValue = QString();
            for (us::USetting* pSetting : printer_extruder_settings)
            {
                if (pSetting->key() == setting->key())
                {
                    pcs->defaultValue = pSetting->str();
                }
            }
            for (us::USetting* pSetting : printer_extruder_user_settings)
            {
                if (pSetting->key() == setting->key())
                {
                    pcs->defaultValue = pSetting->str();
                }
            }
            pcs->currentValue = setting->str();
            m_usNeed->insert(setting);
            paramCheckList.append(pcs);
        }

        return paramCheckList;
    }

    QList<QObject*> PrintMachine::generateMaterialParamCheck()
    {
        PrintProfile* pp = qobject_cast<PrintProfile*>(currentProfileObject());
        us::USettings* us = pp->userSettings();

        //耗材：温度，冷却相关的参数修改
        QList<us::USetting*> cool_settings = us->materialParameters("cool");;
        QList<us::USetting*> temp_settings = us->materialParameters("temperature");
        QList<us::USetting*> profile_material_settings = cool_settings + temp_settings;

        PrintExtruder* pe = qobject_cast<PrintExtruder*>(extruderObject(0));
        int mIndex = pe->materialIndex();
        if (mIndex == -1)
            return QList<QObject*>();

        PrintMaterial* pm = qobject_cast<PrintMaterial*>(materialObject(mIndex));
        //判断耗材配置里面是否有做用户修改
        QList<us::USetting*> material_settings = pm->settings()->materialParameters("cool") +
            pm->settings()->materialParameters("temperature");

        QList<us::USetting*> material_user_settings = pm->userSettings()->materialParameters("cool") +
            pm->userSettings()->materialParameters("temperature");

        QList<QObject*> paramCheckList;
        for (us::USetting* setting : profile_material_settings)
        {
            ParamCheckStruct* pcs = new ParamCheckStruct();
            pcs->keyStr = setting->label();
            pcs->defaultValue = QString();
            for (us::USetting* pSetting : material_settings)
            {
                if (pSetting->key() == setting->key())
                {
                    pcs->defaultValue = pSetting->str();
                }
            }
            //用户修改覆盖默认
            for (us::USetting* pSetting : material_user_settings)
            {
                if (pSetting->key() == setting->key())
                {
                    pcs->defaultValue = pSetting->str();
                }
            }
            pcs->currentValue = setting->str();
            paramCheckList.append(pcs);
            m_usNeed->insert(setting);
        }
        return paramCheckList;
    }
	
    QList<MaterialMeta> PrintMachine::materialMetasInMachine()
    {
        QList<MaterialMeta> defaultMaterials = materialMetas();
        QList<MaterialMeta> machineMaterials;
        loadMaterialMeta(machineMaterials, true, uniqueName());

        return defaultMaterials + machineMaterials;
    }

    PrintMaterial* PrintMachine::_findMaterial(const QString& materialName)
    {
        return cxkernel::findObject<PrintMaterial>(materialName, m_materials);
    }

    void PrintMachine::_cacheSelectMaterials(const int& index)
    {
        if (index == 0)
        {
            writeCacheSelectMaterials(uniqueName(), m_selectMaterials_0, index);
        }
        else if (index == 1)
        {
            writeCacheSelectMaterials(uniqueName(), m_selectMaterials_1, index);
        }
    }

    void PrintMachine::_cacheExtruderMaterialIndex()
    {
        QStringList names = materialsNameList();
        int count = m_extruders.count();
        for (int i = 0; i < count; ++i)
        {
            PrintExtruder* extruder = m_extruders.at(i);
            writeCacheExtruderMaterialIndex(uniqueName(), i, extruder->materialName());
        }
    }

    int PrintMachine::_caculateLevel(const QString& name)
    {
        int level = 0;
        if (name.contains(".default"))
        {
            if (name.contains("best"))
                level = 1;
            else if (name.contains("high"))
                level = 2;
            else if (name.contains("middle"))
                level = 3;
            else if (name.contains("low"))
                level = 4;
            else if (name.contains("fast"))
                level = 5;
        }
        else {
            level = 5 + (int)name.at(0).unicode();
        }

        return level;
    }

    void PrintMachine::save()
    {
        saveSetting(userMachineFile(uniqueName(), false));
    }

    QString creative_kernel::PrintMachine::_getCurrentProfile()
    {
        //1.配置文件保存的当前材料类型
        QString curProfileName = readCacheCurrentProfile(uniqueName());
        int index = 0;
        PrintProfile* curProfile = nullptr;
        if (!m_profiles.count())
        {
            return QString();
        }
        if (!m_profiles.count())
        {
            return QString();
        }
        for (auto profile : m_profiles)
        {
            if (profile->uniqueName() == curProfileName)
            {
                m_currentProfile = profile;
                break;
            }
            ++index;
        }

        for (auto profile : m_profiles)
        {
            if (profile->uniqueName() == "low")
            {
                curProfile = profile;
            }
        }

        //2.选中正常的材料
        if (index == m_profiles.count() && curProfile)
        {
            m_currentProfile = curProfile;
        }
        else if(index == m_profiles.count() && !curProfile){
            //3.选中第一个材料
            m_currentProfile = m_profiles.at(0);
        }
   
        emit curProfileIndexChanged();
        emit isCurrentProfileDefaultChanged();
        return curProfileName;
    }

    Q_INVOKABLE QString creative_kernel::PrintMachine::getFileNameByPath(const QString& path)
    {
        //1.判断文件是否存在
        QUrl url(path);
        QFileInfo info(url.toLocalFile());
        QString baseName = info.baseName();
        return baseName;
    }

    Q_INVOKABLE QString creative_kernel::PrintMachine::getMaterialType(const QString& path)
    {
        //1.判断文件是否存在
        QUrl url(path);
        QFile materialFile(url.toLocalFile());
        if (!materialFile.exists())
        {
            qDebug() << "文件不存在";
            return QString();
        }
        if (!materialFile.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            return QString();
        }
        //2.获取文件内部保存的材料类型
        QString materialType = "";
        while (!materialFile.atEnd())
        {
            QByteArray line = materialFile.readLine();
            QString qLine(line);
            if (qLine.contains("material_type"))
            {
                materialType = qLine.mid(QString("material_type=").length());
            }
        }
        materialFile.close();
        return materialType;
    }

    QList<us::USetting*> creative_kernel::PrintMachine::currentMaterialCover()
    {
        PrintExtruder* pe = qobject_cast<PrintExtruder*>(extruderObject(0));
        int mIndex = pe->materialIndex();
        if (mIndex == -1)
            return QList<us::USetting*>();

        PrintMaterial* pm = qobject_cast<PrintMaterial*>(materialObject(mIndex));
        QList<us::USetting*> coverUser_settings = pm->userSettings()->materialParameters("override");


        PrintProfile* pp = qobject_cast<PrintProfile*>(currentProfileObject());
        us::USettings* us = pp->userSettings();

        QList<us::USetting*> uSettingList;
        bool isProfileChanged = true;
        for (us::USetting* uSetting : coverUser_settings)
        {
            if (!us->findSetting(uSetting->key()))
            {
                uSettingList << uSetting;
            }
        }
        return uSettingList;
    }
	
    QStringList PrintMachine::materialsNameList()
    {
        QStringList names = cxkernel::objectUniqueNames(m_materials);
        return names;
    }
    void PrintMachine::sortMaterials()
    {
        qSort(m_materials.begin(), m_materials.end(), [](PrintMaterial* s1, PrintMaterial* s2) {
            QStringList defaultMateral = QStringList() << "Hyper PLA" << "Hyper PLA-CF" << "Hyper ABS" << "CR-PLA" << "Ender-PLA" << "Generic-PLA" << "Generic-PLA-Silk" << "Generic-PLA-CF" << "CR-PETG" << "Generic-PETG" << "CR-TPU" << "Generic-TPU" << "CR-ABS" << "Generic-ABS" << "Generic-PC" << "HP-ASA" << "Generic-ASA" << "Generic-PA-CF";
            int a = defaultMateral.indexOf(s1->name());

            int b = defaultMateral.indexOf(s2->name());

            if (s1->isUserDef() || s2->isUserDef())
            {
                if (!(s1->isUserDef() && s2->isUserDef()))
                {
                    return s1->isUserDef() ? false : true;
                }
                a = 0;
                b = 0;
            }
            if (a == b && s1->name().length() > 0 && s2->name().length() > 0)
            {
                if (s1->isUserDef() && s2->isUserDef())
                {
                    return s1->getTime() < s2->getTime();
                }
                else {
                    return strcmp(s1->name().toLatin1().constData(), s2->name().toLatin1().constData()) > 0;
                }
                
                /*
                if (s1->name().at(0).cell() - s2->name().at(0).cell() == 0)
                {
                    return s1->name().at(s1->name().length() - 1).cell() - s2->name().at(s2->name().length() - 1).cell()<0;
                }
                else {
                    return s1->name().at(0).cell() - s2->name().at(0).cell()>0;
                }
                */
            }
            if (a >= 0 && b >= 0)
            {
                return a < b;
            }
            else {
                return a > b;
            }
            
            });
    }
    QStringList PrintMachine::materialsName(int extureIndex)
    {
        
        QStringList names = cxkernel::objectUniqueNames(m_materials);
        int index = 0;
        for (QString &name : names)
        {
            if (name.contains("Generic"))
            {
                name.replace("Generic", QCoreApplication::translate("PrintMachine", "Generic"));
            }
            ++index;
        }
        return names;
    }

    QStringList PrintMachine::selectMaterialsName(int extureIndex)
    {
        if (extureIndex == 0)
        {
            return m_selectMaterials_0;
        }
        else if (extureIndex == 1)
        {
            return m_selectMaterials_1;
        }
    }

    QStringList PrintMachine::userMaterialsName()
    {
        QStringList userMaterials;
        for (auto data : m_UserMaterialsData)
        {
            userMaterials.append(data.name);
        }
        return userMaterials;
    }

    QStringList PrintMachine::materialsNameInMachine()
    {
        QStringList namesList;
        auto metas = materialMetasInMachine();
		for (auto meta : metas)
			namesList << meta.name;
        return namesList;
    }

}

