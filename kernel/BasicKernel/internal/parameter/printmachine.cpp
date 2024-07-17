
#include "printmachine.h"

#include "internal/parameter/printextruder.h"
#include "internal/parameter/printprofile.h"
#include "internal/parameter/parametercache.h"
#include "internal/parameter/printmaterial.h"
#include "internal/models/printprofilelistmodel.h"
#include "internal/models/printmateriallistmodel.h"
#include "interface/commandinterface.h"
#include "internal/models/extruderlistmodel.h"
#include "interface/appsettinginterface.h"

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
#include "us/usettingwrapper.h"
#include "external/interface/printerinterface.h"

#include "rapidjson/writer.h"
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"

#define MATERIAL_COLORS_NUM     16

namespace creative_kernel
{
    static QColor colors[MATERIAL_COLORS_NUM] = {
    QColor("#3DDF56"),
    QColor("#FFFFFF"),
    QColor("#F5E077"),
    QColor("#FE6263"),
    QColor("#1B04AE"),
    QColor("#BEBEBE"),
    QColor("#FE37AE"),
    QColor("#6C84FF"),
    QColor("#CE59F8"),
    QColor("#EBFD1B"),
    QColor("#9CFE4F"),
    QColor("#26EEEE"),
    QColor("#000000"),
    QColor("#00A3FF"),
    QColor("#F33910"),
    QColor("#FFA800")
    };

	PrintMachine::PrintMachine(QObject* parent, const MachineData& data, PrintType pt)
		: ParameterBase(parent), m_Pt(pt), m_usNeed(new us::USettings())
		, m_data(data)
	{
        m_PrintMaterialModel = new PrintMaterialModel();
        if (!m_ModelProxy)
        {
            m_ModelProxy = new PrintMaterialModelProxy();
            m_ModelProxy->setSourceModel(m_PrintMaterialModel);
            QQmlEngine::setObjectOwnership(m_ModelProxy, QQmlEngine::QQmlEngine::CppOwnership);
        }

        connect(getKernel()->materialCenter(), &MaterialCenter::materialsChanged, this, &PrintMachine::filterMeterialsFromTypes);
	}

	PrintMachine::~PrintMachine()
	{

	}

	QString PrintMachine::name()
	{
        QString nozzles = "";
        for (int i = 0; i < m_data.extruderCount; ++i)
        {
            if (i == 0)
            {
                nozzles += QString("-%1").arg(m_data.extruderDiameters.at(i));
            }
            else if (i == 1)
            {
                nozzles += QString("+%1").arg(m_data.extruderDiameters.at(i));
            }
        }
        return m_data.baseName + nozzles + " nozzle";
	}

    QString PrintMachine::baseName()
    {
        return m_data.baseName;
    }


    QString PrintMachine::codeName()
    {
        return m_data.codeName;
    }

    bool PrintMachine::isDefault()
    {
        return !m_data.isUser;
    }

    void PrintMachine::setIsDefault(bool isUser)
    {
    }

    void PrintMachine::reload() {
        m_profiles.clear();
        m_extruders.clear();
        m_materials.clear();

        added();

        if (m_BasicDataModel) {
            m_BasicDataModel->deleteLater();
            m_BasicDataModel = nullptr;
            dataModelChanged();
        }

        if (m_extrudersModel) {
            m_extrudersModel->deleteLater();
            m_extrudersModel = nullptr;
            extrudersModelChanged();
        }
    }

    void PrintMachine::strChanged(const QString& key, const QString& str)
    {
        if (key == "extruder_clearance_radius")
        {
            emit extruderClearanceRadiusChanged(str.toFloat());
        }
        else if (key == "extruder_clearance_height_to_lid")
        {
            emit extruderClearanceHeightToLidChanged(str.toFloat());
        }
        else if (key == "extruder_clearance_height_to_rod")
        {
            emit extruderClearanceHeightToRodChanged(str.toFloat());
        }
    }

    float PrintMachine::machineExtruderDiameters()
    {
        return m_data.extruderDiameters.at(0);
    }

    QString PrintMachine::uniqueName()
    {
        return m_data.uniqueName();
    }

    QString PrintMachine::uniqueShowName()
    {
        return m_data.uniqueShowName();
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
        return m_data.isUser;
    }

    bool PrintMachine::isImportedMachine()
    {
        return m_data.is_import;
    }

    bool PrintMachine::centerIsZero()
    {
		if (m_user_settings->hasKey("machine_center_is_zero"))
		{
			return m_user_settings->vvalue("machine_center_is_zero").toBool();
		}
		return m_settings->vvalue("machine_center_is_zero").toBool();
    }

    float PrintMachine::machineWidth()
    {
        return get_machine_width(m_settings, m_user_settings);
    }

    float PrintMachine::machineDepth()
    {
        return get_machine_depth(m_settings, m_user_settings);
    }

    float PrintMachine::machineHeight()
    {
        return get_machine_height(m_settings, m_user_settings);
    }

    int PrintMachine::currentPlateIndex()
    {
        return m_plateIndex;
    }

    void PrintMachine::setCurrentPlateIndex(const int& index)
    {
        if (m_plateIndex == index)
        {
            return;
        }
        m_plateIndex = index;
    }

    void PrintMachine::setPrintSequence(const QString& print_sequence)
    {
        if (print_sequence.isEmpty())
        {
            return;
        }
        m_PlateSetting->add("print_sequence", print_sequence, true);
    }

    void PrintMachine::setCurrBedType(const QString& curr_bed_type)
    {
        if (curr_bed_type.isEmpty())
        {
            return;
        }
        m_PlateSetting->add("curr_bed_type", curr_bed_type, true);
        emit currentBedTypeChanged(curr_bed_type);
    }

    void PrintMachine::setFirstLayerPrintSequence(const QString& first_layer_print_sequence)
    {
        if (first_layer_print_sequence.isEmpty())
        {
            return;
        }
        m_PlateSetting->add("first_layer_print_sequence", first_layer_print_sequence, true);
    }

    void PrintMachine::resetPlateSettings()
    {
        m_PlateSetting->clear();
    }

    bool PrintMachine::enablePrimeTower()
    {
        return m_currentProfile ? get_enable_prime_tower(m_currentProfile->settings(), m_currentProfile->userSettings()) : false;
    }

    float PrintMachine::currentProcessLayerHeight()
    {
        return m_currentProfile ? get_layer_height(m_currentProfile->settings(), m_currentProfile->userSettings()) : 0.0f;
    }

    float PrintMachine::currentProcessPrimeVolume()
    {
        return m_currentProfile ? get_prime_volume(m_currentProfile->settings(), m_currentProfile->userSettings()) : 0.0f;
    }

    float PrintMachine::currentProcessPrimeTowerWidth()
    {
        return m_currentProfile ? get_prime_tower_width(m_currentProfile->settings(), m_currentProfile->userSettings()) : 0.0f;
    }

    bool PrintMachine::enableSupport()
    {
        return m_currentProfile ? get_enable_support(m_currentProfile->settings(), m_currentProfile->userSettings()) : false;
    }

    int PrintMachine::currentProcessSupportFilament()
    {
        return m_currentProfile ? get_support_filament(m_currentProfile->settings(), m_currentProfile->userSettings()) : 0;
    }

    int PrintMachine::currentProcessSupportInterfaceFilament()
    {
        return m_currentProfile ? get_support_interface_filament(m_currentProfile->settings(), m_currentProfile->userSettings()) : 0;
    }

    float PrintMachine::extruderClearanceRadius()
    {
        return get_extruder_clearance_radius(m_settings, m_user_settings);
    }

    float PrintMachine::extruderClearanceHeightToLid()
    {
        return get_extruder_clearance_height_to_lid(m_settings, m_user_settings);
    }

    float PrintMachine::extruderClearanceHeightToRod()
    {
        return get_extruder_clearance_height_to_rod(m_settings, m_user_settings);
    }

    float PrintMachine::initialLayerPrintHeight()
    {
        return m_currentProfile ? get_initial_layer_print_height(m_currentProfile->settings(), m_currentProfile->userSettings()) : 0.0f;
    }

    float PrintMachine::minLayerHeight()
    {
        auto* dataModel = getExtruder1DataModel();
        if (!dataModel) {
            return m_settings->vvalue("min_layer_height").toFloat();
        }

        return dataModel->findOrMakeDataItem("min_layer_height")->getValue().toFloat();
    }

    float PrintMachine::maxLayerHeight()
    {
        auto* dataModel = getExtruder1DataModel();
        if (!dataModel) {
            return m_settings->vvalue("min_layer_height").toFloat();
        }

        return dataModel->findOrMakeDataItem("max_layer_height")->getValue().toFloat();
    }
    QString PrintMachine::_printSequence()
    {
        if (m_currentProfile)
        {
            ParameterDataModel* model = m_currentProfile->getDataModel();
            return model ? model->value("print_sequence") : "";
        }
        return "";
    }
    float PrintMachine::nozzle_volume()
    {
        if (m_user_settings->hasKey("nozzle_volume"))
        {
            return m_user_settings->vvalue("nozzle_volume").toFloat();
        }
        return m_settings->vvalue("nozzle_volume").toFloat();
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
    float PrintMachine::currentNozzleSize()
    {
        bool is_orca = getEngineType() == EngineType::ET_ORCA;
        if (is_orca)
        {
            if (m_user_settings->hasKey("nozzle_diameter"))
            {
                return m_user_settings->vvalue("nozzle_diameter").toFloat();
            }
            return m_settings->vvalue("nozzle_diameter").toFloat();
        }
        else if (m_user_settings->hasKey("machine_nozzle_size"))
        {
            return m_user_settings->vvalue("machine_nozzle_size").toFloat();
        }
        return m_settings->vvalue("machine_nozzle_size").toFloat();
    }
    void PrintMachine::added(bool isInitialize)
    {
        //1.打印机settings读取
        const auto& machine_unique_name = uniqueName();
        QList<us::USettings*>* extruderSettings = new QList<us::USettings*>;
        us::USettings* uSettings = new us::USettings;
        createMachineSettings(machine_unique_name, uSettings, extruderSettings, m_data.isUser, &m_data);
        ;
        if (m_data.inherits_from.isEmpty() || !QFile::exists(defaultMachineFile(m_data.inherits_from)))
        {
            m_data.inherits_from = machine_unique_name;
            m_inherits_from = machine_unique_name;
        }
        setInheritFrom(m_data.inherits_from);

        //读取机型下面用户添加的材料
        m_UserMaterialsData.clear();
        loadMaterialMeta(m_UserMaterialsData, true, inheritsFrom());

        auto preferredProcess = readCacheCurrentProfile(machine_unique_name);
        if (preferredProcess.isEmpty() && !m_data.preferredProcess.isEmpty())
        {
            writeCacheCurrentProfile(machine_unique_name, m_data.preferredProcess);
        }
        setSettings(uSettings);
        setUserSettings(createMachineCoverSettings(machine_unique_name));
        //m_settings->add("machine_nozzle_size", QString("%1").arg(m_data.extruderDiameters.at(0)));
        m_PlateSetting = new us::USettings();
        QStringList plate_keys;
        plate_keys << "print_sequence" << "curr_bed_type" << "first_layer_print_sequence";
        m_PlateSetting->appendDefault(plate_keys);

        //2.喷嘴耗材的读取
        std::vector<QColor> extruderColors;
        std::vector<bool> extruderPhysicals;
        int count = readCacheMachineExtruders(uniqueName(), extruderColors, extruderPhysicals);

        QVariantList colorList;
        for (int i = 0; i < count; ++i)
        {
            bool physical = extruderPhysicals.size() > i ? extruderPhysicals[i] : true;
            QColor color = extruderColors.size() > i ? extruderColors[i] : colors[i];
            colorList << color;
            us::USettings* settings = extruderSettings->isEmpty() ? nullptr : extruderSettings->at(extruderSettings->count() > i ? i : 0)->copy();
            PrintExtruder* extruder = new PrintExtruder(uniqueName(), i, this, physical, color, settings);

            extruder->added();
            m_extruders.append(extruder);

            QList<MaterialData> metas = materialMetasInMachine();
             //materials
            QStringList selectMaterialNames;
            _getSelectMaterials(selectMaterialNames);

            qDebug() << QString("PrintMachine::added ");
            qDebug() << selectMaterialNames;
            qDebug() << uniqueName();
            qDebug() << m_data.supportMaterialDiameters;
            qDebug() << m_data.supportMaterialTypes;

            m_materials.clear();
            for (const QString& selectMaterialName : selectMaterialNames)
            {
               MaterialData meta = findMetaByName(selectMaterialName);
               if(meta.name=="")
               {
                    meta = findMetaByFile(selectMaterialName);
                    if (meta.name == "")
                    {
                        meta.name = selectMaterialName.mid(0,selectMaterialName.lastIndexOf("-"));
                        meta.isUserDef = true;
                    }
               }
               QString materialUName = meta.uniqueName();
               if (selectMaterialName.compare(materialUName, Qt::CaseInsensitive) == 0)
               {
                   MaterialData data;
                   data.id = meta.id;
                   data.brand = meta.brand;
                   data.type = meta.type;
                   data.name = meta.name;
                   data.diameter = meta.diameter;
                   data.rank = meta.rank;
                   data.isUserDef = meta.isUserDef;
                   PrintMaterial* material = new PrintMaterial(inheritsFrom(), data, this);
                   bool res = connect(this, &PrintMachine::currentBedTypeChanged, material, &PrintMaterial::onCurrentBedTypeChanged);

                   material->added();
                   auto it = m_materials.begin();
                   for (; it != m_materials.end(); it++)
                   {
                       if ((*it)->isUserDef())
                       {
                           continue;
                       }
                       if (data.rank > (*it)->rank())
                       {
                           it = m_materials.insert(it, material);
                           break;
                       }
                   }
                   if (it == m_materials.end())
                   {
                       if (data.isUserDef)
                       {
                           m_materials.push_front(material);
                       }
                       else
                       {
                           m_materials.insert(it, material);
                       }
                   }

                   if (i == 0)
                   {
                       m_selectMaterials_0.append(data.uniqueName());
                   }
                   else if (i == 1)
                   {
                       m_selectMaterials_1.append(data.uniqueName());
                   }

                   emit materialsNameChanged();
               }
               else
               {
                    MaterialData data;
                    data.brand = "";
                    data.type = "";
                    data.name = selectMaterialName.left(selectMaterialName.lastIndexOf("-"));
                    data.diameter = 1.75f;
                    data.isUserDef = true;
                    PrintMaterial* material = new PrintMaterial(uniqueName(), data, this);
                    material->added();
                    if(material->isUserDef())
                    {
                        m_materials.push_front(material);
                    }else{
                        m_materials.push_back(material);
                    }

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
               }
            }
            if (m_materials.isEmpty())
            {
                continue;
            }
            //if (isInitialize)
            {
                //filterMeterialsFromTypes();
            }

            QStringList names = materialsNameList();
            QString materialName = readCacheExtruderMaterialIndex(uniqueName(), i);
            if (!names.contains(materialName))
                materialName = names.at(0);

            extruder->setMaterial(materialName);
        }

        //3.工艺配置的读取
        QStringList userFileNames = fetchUserProfileNames(inheritsFrom());
        std::sort(userFileNames.begin(), userFileNames.end(), [this](const QString& s1, const QString& s2) {
            int level1 = _caculateLevel(s1);
            int level2 = _caculateLevel(s2);
            return level1 < level2;
            });
        for (const QString& fileName : userFileNames)
        {
            PrintProfile* profile = new PrintProfile(inheritsFrom(), fileName, this, false);
            profile->added();
            us::USetting *setting = profile->settings()->findSetting("from");
            if(setting)
            {
                if(setting->str()=="system")
                {
                    profile->setDefalut(true);
                }
            }
            m_profiles.append(profile);
        }

        QStringList defaultFileNames = fetchOfficialProfileNames(inheritsFrom());
        for (const QString& fileName : defaultFileNames)
        {
            PrintProfile* profile = new PrintProfile(inheritsFrom(), fileName, this, true);
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

        _cacheExtruderMaterialIndex();

        setFilamentColor(colorList);

        if (m_profilesModel) {
            m_profilesModel->deleteLater();
        }
        m_profilesModel = new PrintProfileListModel{ this, this };
        QQmlEngine::setObjectOwnership(m_profilesModel, QQmlEngine::QQmlEngine::CppOwnership);

        if (m_materialsModel) {
            m_materialsModel->deleteLater();
        }
        m_materialsModel = new PrintMaterialListModel{ this, this };
        QQmlEngine::setObjectOwnership(m_materialsModel, QQmlEngine::QQmlEngine::CppOwnership);

        emit curProfileIndexChanged();
        profilesModelChanged();
        materialsModelChanged();
    }

    void PrintMachine::addImport()
    {
        //setSettings(createMachineSettings(uniqueName()));
        setUserSettings(createMachineCoverSettings(uniqueName()));
        m_settings->add("machine_nozzle_size", QString("%1").arg(m_data.extruderDiameters.at(0)));

        //profiles
        QStringList fileNames;
        if (m_data.isUser)
        {//用户自定义机型
            fileNames = fetchUserProfileNames(uniqueName());
        }
        else {//默认机型
            fileNames = fetchOfficialProfileNames(uniqueName());
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
            QStringList selectMaterialNames;

            if (selectMaterialNames.isEmpty())
            {
                selectMaterialNames = m_data.supportMaterialNames;
            }

            const QList<MaterialData>& metas = materialMetas();

            if (selectMaterialNames.isEmpty())
            {
                for (const MaterialData& meta : metas)
                {
                    if (m_data.supportMaterialTypes.contains(meta.type))
                    {
                        selectMaterialNames.push_back(meta.name);
                    }
                }
            }

            qDebug() << QString("PrintMachine::added ");
            qDebug() << selectMaterialNames;
            qDebug() << uniqueName();
            qDebug() << m_data.supportMaterialDiameters;
            qDebug() << m_data.supportMaterialTypes;

            m_materials.clear();
            for (const QString& selectMaterialName : selectMaterialNames)
            {
                for (const MaterialData& meta : metas)
                {
                    if (selectMaterialName == meta.uniqueName() || selectMaterialName == meta.name)
                    {
                        MaterialData data;
                        data.brand = meta.brand;
                        data.type = meta.type;
                        data.name = meta.name;
                        data.diameter = meta.diameter;
                        data.isUserDef = meta.isUserDef;
                        PrintMaterial* material = new PrintMaterial(uniqueName(), data, this);
                        material->added();
                        if(material->isUserDef())
                        {
                            m_materials.push_front(material);
                        }else{
                            m_materials.push_back(material);
                        }
                        if (m_materialsModel) {
                            m_materialsModel->insertMaterial(material, m_materials.indexOf(material));
                        }
                        emit materialsNameChanged();
                    }
                }
            }
            if (m_materials.isEmpty())
            {
                continue;
            }

            QStringList names = materialsNameList();
            QString materialName = readCacheExtruderMaterialIndex(uniqueName(), i);
            if (!names.contains(materialName))
                materialName = names.at(0);

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

        _cacheExtruderMaterialIndex();
    }

    void PrintMachine::reset()
    {
        for (const auto& key : m_user_settings->settings().keys())
        {
            if(m_BasicDataModel!=nullptr)
                m_BasicDataModel->resetValue(key);
        }
        resetExtruders();

        ParameterBase::reset();
    }

    void PrintMachine::mergeAndSaveExtruders()
    {
        for (auto extruder : m_extruders)
        {
            if (!extruder->physical())
            {
                continue;
            }
            extruder->settings()->merge(extruder->userSettings());
        }
        resetExtruders();
    }

    void PrintMachine::resetExtruders()
    {
        for (auto& extruder : m_extruders) {
            if (extruder) {
                extruder->reset();
            }
        }
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

    ParameterDataModel* PrintMachine::getExtruderDataModel(size_t index) {
        if (extruderCount() < index + 1) {
            return nullptr;
        }

        PrintExtruder* extruder = m_extruders.at(index);
        if (!extruder) {
            return nullptr;
        }

        return extruder->getDataModel();
    }

    ParameterDataModel* PrintMachine::getDataModel()
    {
        if (!m_BasicDataModel)
        {
            m_BasicDataModel = new ParameterDataModel(m_settings, m_user_settings, this);
            QQmlEngine::setObjectOwnership(m_BasicDataModel, QQmlEngine::QQmlEngine::CppOwnership);
        }

        return m_BasicDataModel;
    }

    QObject* PrintMachine::machineParameterModel()
    {
        return getDataModel();
    }

    ParameterDataModel* PrintMachine::getExtruder1DataModel() {
        return getExtruderDataModel(0);
    }

    ParameterDataModel* PrintMachine::getExtruder2DataModel() {
        return getExtruderDataModel(1);
    }

    Q_INVOKABLE QObject* PrintMachine::extruder1DataModel()
    {
        return getExtruder1DataModel();
    }

    Q_INVOKABLE QObject* PrintMachine::extruder2DataModel()
    {
        return getExtruder2DataModel();
    }

    QAbstractListModel* PrintMachine::profilesModel()
    {
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
            //return m_emptyProfile;
            return nullptr;

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

    PrintProfile* PrintMachine::currentProfile()
    {
        return m_currentProfile;
    }

    QObject* PrintMachine::currentProfileObject()
    {
        return m_currentProfile;
    }

    QString PrintMachine::getProcessValue(const QString& processName, const QString& key)
    {
        for (const auto& process : m_profiles)
        {
            if (process->name() == processName)
            {
                return process->settings()->value(key, "");
            }
        }
        return QString();
    }

    bool PrintMachine::isModified() {
        if (m_BasicDataModel && m_BasicDataModel->hasSettingModifyed()) {
            return true;
        }

        auto* extruder1_data_model = getExtruder1DataModel();
        if (extruder1_data_model && extruder1_data_model->hasSettingModifyed()) {
            return true;
        }

        auto* extruder2_data_model = getExtruder1DataModel();
        if (extruder2_data_model && extruder2_data_model->hasSettingModifyed()) {
            return true;
        }

        return false;
    }

    bool PrintMachine::isMaterialModified()
    {
        for (const auto& material : m_materials)
        {
            if (!material->userSettings()->isEmpty())
            {
                return true;
            }
        }
        return false;
    }

    QString PrintMachine::modifiedMaterialName()
    {
        for (const auto& material : m_materials)
        {
            if (!material->userSettings()->isEmpty())
            {
                return material->uniqueName();
            }
        }
        return QString();
    }

    QString PrintMachine::modifiedProcessName()
    {
        for (const auto& profile : m_profiles)
        {
            if (!profile->userSettings()->isEmpty())
            {
                return profile->uniqueName();
            }
        }
        return QString();
    }

    PrintProfile* PrintMachine::modifiedProcessObject()
    {
        for (const auto& profile : m_profiles)
        {
            if (!profile->userSettings()->isEmpty())
            {
                return profile;
            }
        }
        return nullptr;
    }

    bool PrintMachine::isCurrentProcessModified()
    {
        if (!m_currentProfile)
        {
            return false;
        }
        return !m_currentProfile->userSettings()->isEmpty();
    }

    void PrintMachine::saveMaterial()
    {
        for (const auto& material : m_materials)
        {
            if (!material->userSettings()->isEmpty())
            {
                material->mergeAndSave();
                material->exportSetting(defaultMaterialFile(inheritsFrom(), material->uniqueName(), material->isUserDef()));
            }
            material->refreshChangedValue();
        }
    }

    void PrintMachine::abandonMaterialMod()
    {
        for (const auto& material : m_materials)
        {
             //material = m_materials[extruder->materialIndex()];
            if (!material->userSettings()->isEmpty())
            {
                material->reset();
            }
        }
    }

    void PrintMachine::abandonProcessMod()
    {
        for (const auto& profile : m_profiles)
        {
            if (!profile)
            {
                continue;
            }
            profile->reset();
        }

    }

    bool PrintMachine::hasMaterial(const QString& name)
    {
        for (const auto& material : m_materials)
        {
            if (material->name() == name || material->uniqueName() == name)
            {
                return true;
            }
        }
        return false;
    }

    bool PrintMachine::isMaterialInFactory(const QString& name)
    {
        for (const auto& material : m_materials)
        {
            if (material->isUserDef())
            {
                continue;
            }
            if (material->name() == name || material->uniqueName() == name)
            {
                return true;
            }
        }
        return false;
    }

    bool PrintMachine::isMaterialInUser(const QString& name)
    {
        for (const auto& material : m_materials)
        {
            if (material->isUserDef())
            {
                if (material->name() == name || material->uniqueName() == name)
                {
                    return true;
                }
            }
        }
        return false;
    }

    bool PrintMachine::isProcessInFactory(const QString& name)
    {
        for (const auto& process : m_profiles)
        {
            if (process->isDefault())
            {
                if (process->name() == name)
                {
                    return true;
                }
            }
        }
        return false;
    }

    bool PrintMachine::isProcessInUser(const QString& name)
    {
        for (const auto& process : m_profiles)
        {
            if (process->isDefault())
            {
                continue;
            }
            if (process->name() == name)
            {
                return true;
            }
        }
        return false;
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

    void PrintMachine::saveCurrentProfile()
    {
        PrintProfile* pp = qobject_cast<PrintProfile*>(modifiedProcessObject());
        pp->mergeAndSave();
        if (pp->isDefault())
        {
            pp->exportSetting(defaultProfileFile(inheritsFrom(), pp->uniqueName()));
        }
        else
        {
            pp->exportSetting(userProfileFile(inheritsFrom(), pp->uniqueName()));
        }
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
        QString dest = userProfileCoverFile(uniqueName(), profileName, pe->materialName());
        QSettings settingDestIni(dest, QSettings::IniFormat);
        QStringList materialKeys;
        QStringList extruderKeys;

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

    void PrintMachine::syncExtruders(const QStringList& rfid, const QVariantList& color)
    {
        int synExtruderCount = rfid.count();
        int curExtruderCount = extruderAllCount();

        if (!rfid.count())
            return;

        while (curExtruderCount > synExtruderCount)
        {
            this->removeExtruder();
            --curExtruderCount;
        }
        while (curExtruderCount < synExtruderCount)
        {
            this->addExtruder();
            ++curExtruderCount;
        }

        for (int index = 0; index < synExtruderCount; ++index)
        {
            QList<PrintExtruder*> extruders = this->extruders();
            auto new_extruder = extruders.at(index);;
            QString materialName = findMaterialByRFID(rfid.at(index));

            //new_extruder->setColor(color.at(index).toString());
            setExtruderColor(index, color.at(index).toString());
            new_extruder->setMaterial(materialName);
        }

        setFilamentColor(color);
        m_extrudersModel->refresh();
    }

    void PrintMachine::reloadMaterial(const QString& materialName)
    {
        int index = -1;
        QString material_unique_name = materialName + "-1.75";
        for (int i = 0; i < m_materials.size(); ++i)
        {
            const auto& data = m_materials[i];
            if (data->name() == material_unique_name)
            {
                index = i;
                if (m_materialsModel) {
                    m_materialsModel->removeMaterial(data, index);
                }
                break;
            }
        }
        if (index != -1)
        {
            m_materials.removeAt(index);
        }

        MaterialData data = findMetaByName(material_unique_name);

        PrintMaterial* material = new PrintMaterial(inheritsFrom(), data, this);
        material->added();
        if (material->isUserDef())
        {
            m_materials.push_front(material);
        }
        else {
            m_materials.push_back(material);
        }
        if (m_materialsModel) {
            m_materialsModel->insertMaterial(material, m_materials.indexOf(material));
        }
        emit materialsNameChanged();
        materialsModelChanged();
    }

    void PrintMachine::reloadProcess(const QString& processName)
    {
        int index = -1;
        for (int i = 0; i < m_profiles.size(); ++i)
        {
            const auto& data = m_profiles[i];
            if (data->uniqueName() == processName)
            {
                index = i;
                break;
            }
        }
        if (index != -1)
        {
            m_profiles.removeAt(index);
        }

        PrintProfile* profile = new PrintProfile(inheritsFrom(), processName, this, false);
        _addProfile(profile);
    }

    int PrintMachine::extruderCount()
    {
        int count = 0;
        for (const auto& item : m_extruders)
        {
            if (item->physical())
            {
                ++count;
            }
        }
        return count;;
    }

    bool PrintMachine::currentMachineSupportExportImage()
    {
        bool have = m_settings->vvalue("has_preview_img").toBool();
        return have;
    }

    bool PrintMachine::currentMachineIsBbl()
    {
        if (m_data.baseName.contains("Bambu"))
            return true;
        return m_data.is_bbl_printer;
    }

    const QList<PrintExtruder*>& PrintMachine::extruders() const {
      return m_extruders;
    }

    QObject* PrintMachine::extruderObject(int index)
    {
        if (index >= 0 && index < m_extruders.count())
            return m_extruders.at(index);
        return nullptr;
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
        if (materialIndex >= m_materials.length())
        {
            return;
        }
        QString materialName = m_materials.at(materialIndex)->uniqueName();
        if (extruderIndex >= 0 && extruderIndex < m_extruders.count())
        {
            PrintExtruder* extruder = m_extruders.at(extruderIndex);
            QStringList names = materialsNameList();
            //names.append(userMaterialsName());
            if (names.contains(materialName))
            {
                writeCacheExtruderMaterialIndex(currentMachineName(), extruderIndex, materialName);
                extruder->setMaterial(materialName);
                m_extrudersModel->refreshItem(extruder->index());
                extruder->setDirty();
                //for (const auto& profile : m_profiles)
                //{
                //    profile->profileParameterModel(extruderCount() == 1);
                //}
                return;
            }
            return;
        }
    }

    int PrintMachine::extruderAllCount()
    {
        return m_extruders.count();
    }

    //void PrintMachine::setExtruderColor(int extruderIndex, const QColor& extruderColor)
    //{
    //    m_extruders[extruderIndex]->setColor(extruderColor);
    //}

    void PrintMachine::setExtruderColor(int extruderIndex, const QString& extruderColor)
    {
        QColor color(extruderColor);
        m_extruders[extruderIndex]->setColor(color);
        m_extrudersModel->refreshItem(extruderIndex);
        modifyCacheMachineExtruder(uniqueName(), extruderIndex, color);
        emit extruderColorChanged();
    }

    QColor PrintMachine::extruderColor(int extruderIndex) const
    {
        if (extruderIndex > m_extruders.count() - 1 || extruderIndex < 0)
        {
            return QColor();
        }

        if (m_extruders[extruderIndex])
        {
            return m_extruders[extruderIndex]->color();
        }
        return QColor();
    }

    ExtruderListModel* PrintMachine::extrudersListModel() {
        if (!m_extrudersModel) {
            m_extrudersModel = new ExtruderListModel(this, this);
            QQmlEngine::setObjectOwnership(m_extrudersModel, QQmlEngine::QQmlEngine::CppOwnership);
        }
        return m_extrudersModel;
    }

    QAbstractListModel* PrintMachine::extrudersModel()
    {
        return extrudersListModel();
    }

    void PrintMachine::addExtruder()
    {
        if (m_extruders.count() >= MATERIAL_COLORS_NUM)
            return;
        PrintExtruder* extruder = new PrintExtruder(uniqueName(), m_extruders.count(), this, false, getNextExtruderColor(), m_extruders.isEmpty()? nullptr : m_extruders[0]->settings()->copy());
        m_extruders.push_back(extruder);
        writeCacheMachineExtruder(uniqueName(), extruder->color(), extruder->physical());
        if (!m_extrudersModel)
        {
            return;
        }
        m_extrudersModel->elmInsertRow();
        emit extruderColorChanged();
        extruderAdded(extruder);
    }

    void PrintMachine::addExtruder(const QString& color, const QString& curMaterial)
    {
        if (m_extruders.count() >= MATERIAL_COLORS_NUM)
            return;
        PrintExtruder* extruder = new PrintExtruder(uniqueName(), m_extruders.count(), this, false, color, m_extruders.isEmpty() ? nullptr : m_extruders[0]->settings()->copy());
        extruder->setMaterial(curMaterial);
        m_extruders.push_back(extruder);
        writeCacheMachineExtruder(uniqueName(), extruder->color(), extruder->physical());
        if (!m_extrudersModel)
        {
            return;
        }
        m_extrudersModel->elmInsertRow();
        emit extruderColorChanged();
        extruderAdded(extruder);
    }

    QString PrintMachine::findMaterialByRFID(const QString& rfid)
    {
        bool isContant = false;
        QString materialName;

        for (auto material : m_materials)
        {
            if (material->rfid() == rfid)
            {
                materialName = material->uniqueName();
                isContant = true;
            }
        }

        if (!isContant)
        {
            for (auto data : m_SupportMeterialsDataList)
            {
                if (data->id == rfid)
                {
                    materialName = data->uniqueName();
                    selectChanged(true, materialName, 0);
                }
            }
        }

        return materialName;
    }

    void PrintMachine::removeExtruder()
    {
        if (!m_extruders.count())
            return;

        PrintExtruder* extruder = m_extruders.back();
        m_extruders.pop_back();
        extruder->removed();
        extruder->deleteLater();
        extruder = nullptr;
        if (!m_extrudersModel)
        {
            return;
        }
        m_extrudersModel->elmRemoveRow();
        removeCacheMachineExtruder(uniqueName());
        emit extruderColorChanged();
        extruderRemoved(extruder);
    }

    void PrintMachine::setFilamentColor(const QVariantList& colorList)
    {
        bool is_orca = getEngineType() == EngineType::ET_ORCA;
        if (is_orca)
        {
            return;
        }
        QString hexColorString;
        auto globalSettings = createCurrentGlobalSettings();
        for (int i = 0; i < colorList.size(); ++i) {
            const QVariant& color = colorList.at(i);
            hexColorString += color.toString();

            if (i != colorList.size() - 1) {
                hexColorString += ",";
            }
        }

        if (globalSettings->hasKey("filament_colour"))
        {
            currentGlobalUserSettings()->add("filament_colour", hexColorString, true);
        }
    }



    QString PrintMachine::getExtruderValue(const QString& key)
    {
        if (m_extruders.isEmpty())
        {
            return QString();
        }
        return m_extruders[0]->settings()->value(key, "");
    }

    QAbstractListModel* PrintMachine::materialsModel()
    {
        return m_materialsModel;
    }

    PrintMaterial* PrintMachine::material(int index) {
        if (index < 0 || index >= m_materials.count()) {
            return nullptr;
        }

        return m_materials.at(index);
    }

    int PrintMachine::materialCount() {
        return m_materials.size();
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
        QString defFileName = materialCoverFile(inheritsFrom(), oldMaterialUName);
        QFile mf(defFileName);
        if (mf.exists())
        {
            if (mf.isOpen())
            {
                mf.close();
            }
            QString newFilePath = materialCoverFile(inheritsFrom(), curMaterialUName);
            bool res = mf.rename(newFilePath);
            int a = 0;
        }

        //3.修改material_user.json里的材料名称
        reNameUserMaterialFromJson(oldMaterialUName, newMaterialName, inheritsFrom());

        //4.修改注册表里的材料名称

        //5.修改机器保存的材料对象列表的材料名称

        //6.刷新修改的内容
        return true;
    }

    QStringList PrintMachine::supportTypes()
    {
        return m_data.supportMaterialTypes;
    }

    QStringList PrintMachine::selectTypes(int extruderIndex)
    {
        QStringList selectMaterialNames;
        _getSupportMaterials(selectMaterialNames);/*readCacheSelectMaterials(uniqueName(), extruderIndex);*/

        if (selectMaterialNames.isEmpty())
        {
            for (QString mName : m_data.supportMaterialNames)
            {
                QString temp = QString("%1_%2").arg(mName).arg(m_data.supportMaterialDiameters.at(0));
                selectMaterialNames.append(temp);
            }
        }

        const QList<MaterialData>& metas = materialMetasInMachine();

        QMultiMap<QString, QString> materialMap;
        for (QString mName : selectMaterialNames)
        {
            for (MaterialData mm : metas)
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

    QObject* PrintMachine::materialObject(int index) const
    {
        return materialAt(index);
    }

    QObject* PrintMachine::materialObject(const QString& materialName) const
    {
        return materialWithName(materialName);
    }

    PrintMaterial* PrintMachine::materialAt(int index) const {
        if (index >= 0 && index < m_materials.count())
            return m_materials.at(index);
        return nullptr;
    }

    PrintMaterial* PrintMachine::materialWithName(const QString& materialName) const {
        for (PrintMaterial* data : m_materials)
        {
            if (data->uniqueName() == materialName)
            {
                return data;
            }
        }
        return nullptr;
    }

    void PrintMachine::filterMeterialsFromTypes()
    {
        m_SupportMeterialsDataList.clear();
        QStringList curgroupFilter;
        const QList<MaterialData>& allMetas = materialMetas();
        QStringList supportMaterials;
        _getSupportMaterials(supportMaterials);
        m_PrintMaterialModel->setSupportMaterialNames(supportMaterials);

       for (int i = 0; i < allMetas.size(); i++)
       {
           QString mType = allMetas.at(i).type.trimmed();
           MaterialData* data = new MaterialData;
           data->name = allMetas.at(i).name;
           data->diameter = allMetas.at(i).diameter;
           data->id = allMetas.at(i).id;

           if (m_selectMaterials_0.indexOf(data->uniqueName()) >= 0)
           {
               data->isChecked = true;
           }

           data->type = mType;
           m_SupportMeterialsDataList.append(data);
           m_PrintMaterialModel->addMaterial(data);
       }
       m_ModelProxy->setSelectMaterialTypes(getKernel()->materialCenter()->types());
       m_ModelProxy->setSourceModel(m_PrintMaterialModel);
    }

    void PrintMachine::setExtruderStatus(bool bextruder0, bool bextruder1)
    {
        int enableCount = (bextruder0 ? 1 : 0) + (bextruder1 ? 1 : 0);
        if (extruderCount() > 1)
        {
            //if (this->m_currentProfile)
            //{
            //    ProfileParameterModel* model = qobject_cast<ProfileParameterModel*>(this->m_currentProfile->profileParameterModel(false));
            //    if (model)
            //    {
            //        model->setValue("extruders_enabled_count", QString::number(enableCount));
            //    }
            //}
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

     Q_INVOKABLE QVariant creative_kernel::PrintMachine::materialsModelProxy()
     {
         return QVariant::fromValue(m_ModelProxy);
     }

    int PrintMachine::addUserMaterial(const QString& materialModel, const QString& userMaterialName, int extruderNum)
    {
        int material_index = -1;
        //更新数据
        PrintMaterial* pMaterial = nullptr;
        //const QList<MaterialData>& metas = materialMetas();
		const QList<MaterialData>& metas = materialMetasInMachine();

        for (auto material : m_materials)
        {
            if (material->uniqueName() == materialModel)
            {
                pMaterial = material;
            }
        }
        if (pMaterial == nullptr)
        {
            return material_index;
        }
        MaterialData data;
        data.brand = pMaterial->brand();
        data.type = pMaterial->type();
        data.name = userMaterialName;
        data.diameter = pMaterial->diameter();
        data.isUserDef = true;
        _addUserMaterialToJson(data);



                QString filePath = defaultMaterialFile(inheritsFrom(), data.uniqueName(), true);

               //如果存在先删除原来的耗材
                if (isMaterialInUser(data.uniqueName()))
                {
                    for (const auto& material : m_materials)
                    {
                            if (material->uniqueName() == data.uniqueName())
                            {
                                if (m_materialsModel) {
                                    m_materialsModel->removeMaterial(material, m_materials.indexOf(material));
                                }
                                    m_materials.removeOne(material);
                                    material->removed();
                                    break;
                            }
                    }
                    //continue;
                }
                //1. 拷贝耗材文件
                pMaterial->exportSetting(filePath);
                pMaterial->reset();

                //2.创建材料对象
                PrintMaterial* material = new PrintMaterial(inheritsFrom(), data, this);
                material->addedUserMaterial();

                //3.添加材料对象
                if(material->isUserDef())
                {
                    m_materials.push_front(material);
                }else{
                    m_materials.push_back(material);
                }
                if (m_materialsModel) {
                    m_materialsModel->insertMaterial(material, m_materials.indexOf(material));
                }
                emit materialsNameChanged();
                material_index = m_materials.indexOf(material);

                material->refreshChangedValue();
        return material_index;

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
                if (m_materialsModel) {
                    m_materialsModel->removeMaterial(materialObj, m_materials.indexOf(materialObj));
                }
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
        removeUserMaterialFromJson(materialName, inheritsFrom());
        //4.删除配置文件
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
        const QList<MaterialData>& metas = materialMetas();

        for (const MaterialData& meta : metas)
        {
            if (materialType.split("_")[0] == meta.name)
            {
                MaterialData data;
                data.brand = meta.brand;
                data.type = meta.type;
                data.name = userMaterialName;
                data.diameter = meta.diameter;
                data.isUserDef = true;

                //1. 拷贝耗材文件
                QString defFileName = materialCoverFile(uniqueName(), data.uniqueName());
                qtuser_core::copyFile(url.toLocalFile(), defFileName, true);

                //2.创建材料对象
                PrintMaterial* material = new PrintMaterial(uniqueName(), data, this);
                material->addedUserMaterial();

                //3.添加材料对象
                m_materials.push_front(material);
                if (m_materialsModel) {
                    m_materialsModel->insertMaterial(material, m_materials.indexOf(material));
                }
                emit materialsNameChanged();

                //4.添加材料到json
                MaterialData metaNew = meta;
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
            }
        }
    }

    void PrintMachine::finishMeterialSelect(const int& index)
    {
        auto func = [this](MaterialData* mData) {
            PrintMaterial* material = nullptr;
            const QList<MaterialData>& metas = materialMetasInMachine();
            for (const MaterialData& meta : metas)
            {
                if (mData->name == meta.name)
                {
                    MaterialData data;
                    data.brand = meta.brand;
                    data.type = meta.type;
                    data.name = meta.name;
                    data.diameter = meta.diameter;
                    data.isUserDef = meta.isUserDef;
                    material = new PrintMaterial(uniqueName(), data, this);
                    break;
                }
            }
            return material;
        };

        m_materials.clear();
        m_selectMaterials_0.clear();
        foreach (auto m, m_SupportMeterialsDataList)
        {
            PrintMaterial* pMaterial = func(m);
            if (!pMaterial)
                continue;

            assert(pMaterial);
            if (m->isChecked && m->isVisible)
            {
                QString fileName = materialCoverFile(uniqueName(), pMaterial->uniqueName());
                pMaterial->added();
                if(pMaterial->isUserDef())
                {
                    m_materials.push_front(pMaterial);
                }else{
                    m_materials.push_back(pMaterial);
                }

                if (m_materialsModel) {
                    m_materialsModel->insertMaterial(pMaterial, m_materials.indexOf(pMaterial));
                }
                m_selectMaterials_0.append(pMaterial->uniqueName());
            }
            else if(!m->isChecked)
            {
                QString fileName = materialCoverFile(uniqueName(), pMaterial->uniqueName());
                QFile file(fileName);
                if (file.exists())
                {
                    file.remove();
                }
            }
        }

        //选中的材料再追加上用户自定义材料
        for (MaterialData um : m_UserMaterialsData)
        {
            MaterialData data;
            data.brand = um.brand;
            data.type = um.type;
            data.name = um.name;
            data.diameter = um.diameter;
            data.isUserDef = um.isUserDef;
            PrintMaterial* material = new PrintMaterial(uniqueName(), data, this);
            material->addedUserMaterial();

            //3.添加材料对象
            if(material->isUserDef())
            {
                    m_materials.push_front(material);
            }else{
                    m_materials.push_back(material);
            }
            if (m_materialsModel) {
                m_materialsModel->insertMaterial(material, m_materials.indexOf(material));
            }
            m_selectMaterials_0.append(um.uniqueName());
        }

        emit materialsNameChanged();
        emit m_extruders[index]->materialIndexChanged();
        //m_extrudersModel->refreshItem(0);
        m_extrudersModel->refresh();
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
        if (m_globalSetting.isNull())
        {
            m_globalSetting = QSharedPointer<us::USettings>(new us::USettings(this));
            m_globalSetting->loadCompleted();
        }
        if (m_currentProfile)
        {
            //if (getEngineType() == EngineType::ET_ORCA)
            //{
            //    m_currentProfile->settings()->add("wipe_tower_x", getAllWipeTowerPositionX(), true);
            //    m_currentProfile->settings()->add("wipe_tower_y", getAllWipeTowerPositionY(), true);
            //}
            m_globalSetting->merge(m_currentProfile->settings());
            m_globalSetting->merge(m_currentProfile->userSettings());
        }
        m_globalSetting->merge(m_settings);
        m_globalSetting->merge(this->m_user_settings);
        bool is_orca = getEngineType() == EngineType::ET_ORCA;
        if (is_orca)
        {
            if (!m_extruders.empty())
            {
                m_globalSetting->merge(m_extruders[0]->settings());
                m_globalSetting->merge(m_extruders[0]->userSettings());
            }
            if (!m_PlateSetting->isEmpty())
            {
                m_globalSetting->merge(m_PlateSetting);
            }
        }

        //m_globalSetting->add("printer_model", m_data.baseName, true);
        return m_globalSetting;
    }

    us::USettings* PrintMachine::currentGlobalSettings()
    {
        return m_user_settings;
    }

    QList<QSharedPointer<us::USettings>> PrintMachine::createCurrentExtruderSettings()
    {
        const static QStringList override_keys = {
            "filament_retraction_length", "filament_z_hop", "filament_z_hop_types", "filament_retract_lift_above",
            "filament_retract_lift_below","filament_retraction_speed", "filament_deretraction_speed", "filament_retract_restart_extra", "filament_retraction_minimum_travel",
            "filament_wipe_distance","filament_retract_lift_enforce",
            "filament_retract_when_changing_layer", "filament_wipe",
            "filament_retract_before_wipe" };
        QList<QSharedPointer<us::USettings>> extruderSettings;
        bool is_orca = getEngineType() == EngineType::ET_ORCA;
        for (PrintExtruder* extruder : m_extruders)
        {
            us::USettings* settings = extruder->settings();
            QSharedPointer<us::USettings> extruderSetting(settings->copy());
            QString materialName = extruder->materialName();
            PrintMaterial* material = _findMaterial(materialName);
            if (material)
            {
                auto material_settings = material->settings()->copy();
                material_settings->merge(material->userSettings());
                for (const auto& key : override_keys)
                {
                    if (!material->userSettings()->hasKey(key))
                    {
                        material_settings->erase(key);
                    }
                }
                extruderSetting->merge(material_settings);
            }
            extruderSetting->merge(extruder->userSettings()->copy());

            if (is_orca)
            {
                extruderSetting->add("filament_colour", extruder->color().name(), true);
            }

            //if (m_extruders.size() == 1)
            //{
            //    const auto& affectedKeys = m_currentProfile->getAffetecdKeys();
            //    for (const auto& profileSettings : m_currentProfile->userSettings()->settings())
            //    {
            //        const auto& key = profileSettings->key();
            //        if (extruderSetting->hasKey(key))
            //        {
            //            extruderSetting->findSetting(key)->setValue(m_currentProfile->getModelValue(key));
            //            if (affectedKeys.contains(key))
            //            {
            //                for (const auto& affectedKey : affectedKeys[key])
            //                {
            //                    QString value = m_currentProfile->getModelValue(affectedKey);
            //                    if (extruderSetting->hasKey(affectedKey))
            //                    {
            //                        extruderSetting->findSetting(affectedKey)->setValue(value);
            //                    }
            //                    else
            //                    {
            //                        extruderSetting->add(affectedKey, value);
            //                    }
            //                }
            //            }
            //        }
            //    }
            //}
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

    void creative_kernel::PrintMachine::_addUserMaterialToJson(const MaterialData& materialMeta)
    {
        //需要实现
        saveMateriaMeta(materialMeta, inheritsFrom());
    }
    void PrintMachine::transferProfile(QObject* from, QObject* to)
    {
        PrintProfile* pp_from = qobject_cast<PrintProfile*>(from);
        PrintProfile* pp_to = qobject_cast<PrintProfile*>(to);
        us::USettings* fus = pp_from->userSettings();
        if (fus)
        {
            auto item = pp_to->getDataModel();
            for (const auto& key : fus->hashSettings().keys())
            {
                auto item = qobject_cast<ParameterDataItem*>(pp_to->getDataModel()->getDataItem(key));
                item->setValue(fus->value(key, ""));
            }
            pp_from->reset();
            pp_to->save();
        }

    }
    void PrintMachine::_addProfile(const QString& profileName, const QString& templateProfile)
    {
        PrintProfile* pp = qobject_cast<PrintProfile*>(modifiedProcessObject());

        for (const auto& profile : m_profiles)
        {
            if (profile->uniqueName() == profileName)
            {
                _removeProfile(profile);
            }
        }

        QString newProfileName = userProfileFile(inheritsFrom(), profileName);
        pp->exportSetting(newProfileName);
        pp->reset();
        PrintProfile* newProfile = new PrintProfile(inheritsFrom(), profileName, this, false);
        _addProfile(newProfile);
    }

    void PrintMachine::_addProfile(PrintProfile* profile)
    {
        if (!profile)
            return;

        profile->added();
        if(profile->isDefault())
        {
            m_profiles.push_back(profile);
        }else{
            m_profiles.push_front(profile);
        }


        if (m_profilesModel) {
            m_profilesModel->insertProfile(profile, m_profiles.indexOf(profile));
        }

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

        if (m_profilesModel) {
            m_profilesModel->removeProfile(profile, m_profiles.indexOf(profile));
        }

        profile->removed();
        m_profiles.removeOne(profile);
        profile->deleteLater();

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
            emit curProfileIndexChanged();
            emit isCurrentProfileDefaultChanged();
            return;
        }

        m_currentProfile = profile;
        QQmlEngine::setObjectOwnership(m_currentProfile, QQmlEngine::QQmlEngine::CppOwnership);
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

    MaterialData PrintMachine::findMetaByName(const QString& machineName)
    {
        MaterialData res;
        QList<MaterialData> metas = materialMetasInMachine();
        for (const MaterialData& meta : metas)
        {
            if (machineName.compare(meta.uniqueName(), Qt::CaseInsensitive) == 0)
            {
                res = meta;
                break;
            }
        }

        return res;
    }
     MaterialData PrintMachine::findMetaByFile(const QString& materialName)
     {
        MaterialData res;
        QString defFileName = QString("%1/%2/Materials/%3.json").arg(_pfpt_user_parampack).arg(uniqueName()).arg(materialName);
        QFileInfo defFileInfo(defFileName);
        if(defFileInfo.exists())
        {
            QFile file(defFileInfo.absoluteFilePath());
            if(!file.open(QIODevice::ReadOnly))
            {
                return res;
            }
            QByteArray jsonData = file.readAll();
            file.close();
            QJsonDocument doc = QJsonDocument::fromJson(jsonData);

            if(!doc.isNull())
            {
                QJsonObject material = doc.object()["engine_data"].toObject();
                QJsonObject metadata = doc.object()["metadata"].toObject();
                res.name = metadata["name"].toString();
                res.type = material["filament_type"].toString();
                res.isUserDef = material["from"].toString()=="system"?false:true;

            }
        }
        return res;
     }


    void PrintMachine::_getSelectMaterials(QStringList& selectMaterialNames)
    {
        const QList<MaterialData>& metas = materialMetasInMachine();
        //1.读取注册表
        //selectMaterialNames = readCacheSelectMaterials(uniqueName(), 0);
        selectMaterialNames = QStringList();

        //2.读取机型支持的耗材名称
        if (selectMaterialNames.isEmpty())
        {
            QString defFileName = QString("%1/%2/Materials").arg(_pfpt_default_parampack).arg(inheritsFrom());
            selectMaterialNames = fetchFileNames(defFileName, true);

            defFileName = QString("%1/%2/Materials").arg(_pfpt_user_parampack).arg(inheritsFrom());
            QStringList userList = fetchFileNames(defFileName, true);
            for (QString fileName : userList)
            {
                if (!fileName.contains(".cover"))
                {
                    selectMaterialNames += fileName;

                }
            }
        }
    }

    void PrintMachine::_getSupportMaterials(QStringList& supportMaterialNames)
    {
        if (m_data.inherits_from.isEmpty())
        {
            supportMaterialNames = fetchFileNames(QString("%1/%2/Materials").arg(_pfpt_default_parampack).arg(uniqueName()), true);
            supportMaterialNames += fetchFileNames(QString("%1/%2/Materials").arg(_pfpt_user_parampack).arg(uniqueName()), true);
        }
        else {
            supportMaterialNames = fetchFileNames(QString("%1/%2/Materials").arg(_pfpt_default_parampack).arg(m_data.inherits_from), true);
            supportMaterialNames += fetchFileNames(QString("%1/%2/Materials").arg(_pfpt_user_parampack).arg(m_data.inherits_from), true);
        }

    }

	QList<QObject*> PrintMachine::generatePrinterParamCheck()
    {
        PrintProfile* pp = qobject_cast<PrintProfile*>(currentProfileObject());
        us::USettings* us = pp->userSettings();
        m_usNeed->clear();

        //配置文件：挤出机相关
        QList<us::USetting*> re_settings;
        QList<us::USetting*> li_settings;
        QList<us::USetting*> profile_extruder_settings = re_settings + li_settings;

        //打印机：挤出机相关
        PrintExtruder* pe = qobject_cast<PrintExtruder*>(extruderObject(0));
        QList<us::USetting*> printer_extruder_settings;
        QList<us::USetting*> printer_extruder_user_settings;
        if (pe)
        {
            printer_extruder_settings;
            printer_extruder_user_settings;
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
        QList<us::USetting*> cool_settings;
        QList<us::USetting*> temp_settings;
        QList<us::USetting*> profile_material_settings = cool_settings + temp_settings;

        PrintExtruder* pe = qobject_cast<PrintExtruder*>(extruderObject(0));
        int mIndex = pe->materialIndex();
        if (mIndex == -1)
            return QList<QObject*>();

        PrintMaterial* pm = qobject_cast<PrintMaterial*>(materialObject(mIndex));
        //判断耗材配置里面是否有做用户修改
        QList<us::USetting*> material_settings;

        QList<us::USetting*> material_user_settings;

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

    QList<MaterialData> PrintMachine::materialMetasInMachine()
    {
        QList<MaterialData> defaultMaterials = materialMetas();
        QList<MaterialData> machineMaterials;
        loadMaterialMeta(machineMaterials, true, inheritsFrom());
        loadMaterialMeta(machineMaterials, false, inheritsFrom());

        return defaultMaterials + machineMaterials;
    }

    PrintMaterial* PrintMachine::_findMaterial(const QString& materialName)
    {
        return cxkernel::findObject<PrintMaterial>(materialName, m_materials);
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

    QColor creative_kernel::PrintMachine::getNextExtruderColor()
    {
        int count = m_extruders.count();
        return colors[count];
    }

    void PrintMachine::save()
    {
        saveSetting(machineCoverFile(uniqueName()));
    }

    void creative_kernel::PrintMachine::exportSetting(const QString& fileName)
    {
        us::USettings* settings = m_settings->copy();
        if (m_user_settings)
            settings->merge(m_user_settings);

        {
            QFile file(fileName);
            file.remove();
        }

        QFile file(fileName);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        {
            qDebug() << QString("USettings::saveAsDefault [%1] failed.").arg(fileName);
            return;
        }

        rapidjson::Document docJsonFile;
        docJsonFile.SetObject();
        auto& allocator = docJsonFile.GetAllocator();

        auto printerObj = rapidjson::Value{ rapidjson::Type::kObjectType };
        for (const auto& key : settings->hashSettings().keys())
        {
            auto key_string = key.toStdString();
            const char* ckey = key_string.c_str();
            auto value_string = settings->value(key, "").toStdString();
            const char* cvalue = value_string.c_str();
            printerObj.AddMember(rapidjson::Value(ckey, allocator), rapidjson::Value(cvalue, allocator), allocator);
        }
        //rapidjson::Value v1(document, allocator);
        docJsonFile.AddMember("printer", printerObj, allocator);

        auto meta_object = rapidjson::Value{ rapidjson::Type::kObjectType };
        auto inherits_from_string = inheritsFrom().toStdString();
        const char* inherits_from = inherits_from_string.c_str();
        meta_object.AddMember(rapidjson::Value("inherits_from", allocator), rapidjson::Value(inherits_from, allocator), allocator);
        docJsonFile.AddMember("metadata", meta_object, allocator);

        auto extruder_array = rapidjson::Value{ rapidjson::Type::kArrayType };
        for (const auto& extruder : m_extruders)
        {
            us::USettings* settings = extruder->settings()->copy();
            if (extruder->userSettings())
                settings->merge(extruder->userSettings());

            auto extruder_object = rapidjson::Value{ rapidjson::Type::kObjectType };
            auto engine_object = rapidjson::Value{ rapidjson::Type::kObjectType };
            for (const auto& key : settings->hashSettings().keys())
            {
                auto key_string = key.toStdString();
                const char* ckey = key_string.c_str();
                auto value_string = settings->value(key, "").toStdString();
                const char* cvalue = value_string.c_str();
                engine_object.AddMember(rapidjson::Value(ckey, allocator), rapidjson::Value(cvalue, allocator), allocator);
            }
            extruder_object.AddMember("engine_data", engine_object, allocator);
            auto meta_object = rapidjson::Value{ rapidjson::Type::kObjectType };
            meta_object.AddMember("color", extruder->color().rgba64().toArgb32(), allocator);

            auto current_material_string = extruder->materialName().toStdString();
            const char* current_material = current_material_string.c_str();
            meta_object.AddMember(rapidjson::Value("current_material", allocator), rapidjson::Value(current_material, allocator), allocator);
            meta_object.AddMember("is_physical", extruder->physical(), allocator);
            extruder_object.AddMember("metadata", meta_object, allocator);
            extruder_array.PushBack(extruder_object, allocator);
            delete settings;
        }
        docJsonFile.AddMember("extruders", std::move(extruder_array), allocator);
        rapidjson::StringBuffer strbuf;
        rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
        docJsonFile.Accept(writer);
        file.write(std::string(strbuf.GetString(), strbuf.GetSize()).c_str(), strbuf.GetSize());
        file.close();

        delete settings;
    }

    QString creative_kernel::PrintMachine::_getCurrentProfile()
    {
        //1.配置文件保存的当前材料类型
        QString curProfileName = readCacheCurrentProfile(uniqueName());
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
                curProfile = profile;
                break;
            }
        }

        //2.选中正常的材料
        if (curProfile)
        {
            m_currentProfile = curProfile;
        }
        else
        {
            //3.选中第一个材料
            m_currentProfile = m_profiles.at(0);
        }
        QQmlEngine::setObjectOwnership(m_currentProfile, QQmlEngine::QQmlEngine::CppOwnership);

        //emit curProfileIndexChanged();
        //emit isCurrentProfileDefaultChanged();
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
        QList<us::USetting*> coverUser_settings;


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

    QStringList PrintMachine::materialsName(int extureIndex)
    {
        QStringList names = cxkernel::objectNames(m_materials);
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

    QStringList PrintMachine::defaultMaterialsName()
    {
        QStringList usermaterials = userMaterialsName();
        QStringList materials = materialsName();
        QStringList res;
        for (QString mm : materials)
        {
            if (!usermaterials.contains(mm))
            {
                res << mm;
            }
        }

        return res;
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

    QStringList PrintMachine::materialDiffKeys(int materialIndex1, int materialIndex2)
    {
        if (materialIndex1 < 0 || materialIndex2 < 0)
            return QStringList();

        QStringList names = cxkernel::objectUniqueNames(m_materials);
        if (names.count() <= 0)
            return QStringList();

        return materialDiffKeys(names.at(materialIndex1), names.at(materialIndex2));
    }

    QStringList PrintMachine::profileDiffKeys(int profileIndex1, int profileIndex2)
    {
        if (profileIndex1 < 0 || profileIndex2 < 0)
            return QStringList();

        QStringList names = profileNames();
        if (names.count() <= 0)
            return QStringList();

        return profileDiffKeys(names.at(profileIndex1), names.at(profileIndex2));
    }

    QStringList PrintMachine::materialDiffKeys(const QString materialName1, const QString materialName2)
    {
        if (materialName1 == materialName2 || materialName1.isEmpty() || materialName2.isEmpty())
            return QStringList();

        PrintMaterial* material1 = nullptr;
        PrintMaterial* material2 = nullptr;
        for (auto material : m_materials)
        {
            QString name1 = material->uniqueName();
            QString name2 = material->name();
            if (material->uniqueName() == materialName1)
                material1 = material;
            else if (material->uniqueName() == materialName2)
                material2 = material;
        }

        QStringList diffKeys = material1->compareSettings(material2);
        return diffKeys;
    }

    QStringList PrintMachine::profileDiffKeys(const QString profileName1, const QString profileName2)
    {
        if (profileName1 == profileName2 || profileName1.isEmpty() || profileName2.isEmpty())
            return QStringList();

        PrintProfile* profile1 = nullptr;
        PrintProfile* profile2 = nullptr;
        for (auto profile : m_profiles)
        {
            QString name1 = profile->uniqueName();
            QString name2 = profile->name();
            if (profile->uniqueName() == profileName1)
                profile1 = profile;
            else if (profile->uniqueName() == profileName2)
                profile2 = profile;
        }

        QStringList diffKeys = profile1->compareSettings(profile2);
        return diffKeys;
    }

    QStringList PrintMachine::materialsNameInMachine()
    {
        QStringList namesList;
        auto metas = materialMetasInMachine();
		for (auto meta : metas)
			namesList << meta.name;
        return namesList;
    }

    QStringList creative_kernel::PrintMachine::nozzleDiameter()
    {
        QStringList namesList;
        for (auto nozzle : m_data.extruderDiameters)
            namesList << QString::number(nozzle, 'f', 1);
        return namesList;
    }

}
