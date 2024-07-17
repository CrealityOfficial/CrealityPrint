#include "internal/parameter/parametermanager.h"

#include <set>

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonValue>
#include <QtCore/QPointer>
#include <QtCore/QStandardPaths>
#include <QtCore/QUrl>
#include <QtGui/QColor>

#include <rapidjson/document.h>
#include <rapidjson/filereadstream.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <cxkernel/interface/iointerface.h>
#include <cxkernel/interface/jobsinterface.h>
#include <cxkernel/utils/traitsutil.h>

#include <qtusercore/module/systemutil.h>
#include <qtusercore/string/resourcesfinder.h>
#include <qtusercore/util/settings.h>

#include "external/interface/appsettinginterface.h"
#include "external/interface/commandinterface.h"
#include "external/interface/uiinterface.h"
#include "external/interface/visualsceneinterface.h"
#include "external/job/nest2djobex.h"
#include "external/kernel/globalconst.h"
#include "external/kernel/kernel.h"
#include "external/kernel/kernelui.h"
#include "external/kernel/translator.h"
#include "external/message/parameterupdateablemessage.h"
#include "external/us/settingdef.h"
#include "external/us/usetting.h"

#include "internal/models/printmachinelistmodel.h"
#include "internal/models/usermachinemodel.h"
#include "internal/parameter/parametercache.h"
#include "internal/parameter/parameterpath.h"
#include "internal/parameter/parameterutil.h"
#include "internal/parameter/printextruder.h"
#include "internal/parameter/printmachine.h"
#include "internal/parameter/printmaterial.h"
#include "internal/parameter/printprofile.h"
#include "internal/multi_printer/printermanager.h"
#include "cxkernel/kernel/cxkernel.h"
#include <cxcloud/service_center.h>
#include <QColorDialog>

#include <QTemporaryDir>
#include "qtusercore/module/quazipfile.h"
#include <QBuffer>
#include "quazip/quazip.h"
#include "quazip/quazipfile.h"
#include "external/us/defaultloader.h"

#include <QDomDocument>
#include <QDomElement>
#include <QDomNode>
#include <external/slice/slicepreviewflow.h>

namespace creative_kernel
{
    static PrintMachine* s_currentMachine;

    ParameterManager::ParameterManager(QObject* parent)
        : QObject{ parent }
        , m_MachineListModel{ new PrintMachineListModel{ this, this } }
        , m_addPrinterModel{ new CusAddPrinterModel{ this } }
        , m_materialCenter{ new MaterialCenter{ this } } {}

    ParameterManager::~ParameterManager() {}

    void ParameterManager::initialize()
    {
        m_addPrinterModel->initialize();

        loadMachineMeta(m_machineDatas);
        loadMachineMeta(m_machineDatas, true);
        loadOfficialMachineNames(m_factoryMachineNames);
        loadUserMachineNames(m_userMachineNames);
        // us::SettingDef::instance().writeAllKeys();

        QStringList machineUniqueNames = readCacheMachineUniqueNames();

        qDebug() << QString("ParameterManager::initialize : ");
        qDebug() << machineUniqueNames;
        for (const QString &machineUniqueName : machineUniqueNames)
        {
            bool isFactory = false;
            bool isUser = false;
            if (m_factoryMachineNames.contains(machineUniqueName))
            {
                isFactory = true;
            }
            else if(m_userMachineNames.contains(machineUniqueName))
            {
                isUser = true;
            }
            else
            {
                continue;
            }
            makeSureUserParamPackDir(machineUniqueName);
            PrintMachine *machine = _createMachine(machineUniqueName);
            if (machine)
            {
                machine->setParent(this);
                machine->added(true);
                if(machine->isDefault())
                {
                    m_machines.push_back(machine);
                }else{
                    m_machines.push_front(machine);
                }

                m_MachineListModel->insertMachine(machine, m_machines.indexOf(machine));
            }
        }
        m_materialCenter->initialize();
        emit machineNameListChanged();
        emit userMachineNameListChanged();
        emit defaultMachineNameListChanged();

        qtuser_core::VersionSettings setting;
        setting.beginGroup("profile_setting");
        QString strStartType = setting.value("first_start", "0").toString();
        if (strStartType != "0")
        {
            emit machinesCountChanged();
        }
        setting.endGroup();

        if (m_machines.count() > 0)
        {
            int currentMachineIndex = readCacheCurrentMachineIndex();
            if (currentMachineIndex < 0 || currentMachineIndex >= m_machines.count())
                currentMachineIndex = 0;

            setCurrentMachine(currentMachineIndex, true);
        }

        _cacheCurrentMachineIndex();

        connect(this, &ParameterManager::parameterEdited, this, &ParameterManager::onParameterEdited);

        connect(this, &ParameterManager::curMachineIndexChanged, this, &ParameterManager::onCurrentMachineChanged);
        connect(this, &ParameterManager::curMachineIndexChanged, this, &ParameterManager::currentColorsChanged);
        if (m_currentMachine)
        {
            PrintProfile* profile = qobject_cast<PrintProfile*>(m_currentMachine->currentProfileObject());
            if (profile)
            {
                connect(profile, &PrintProfile::enablePrimeTowerChanged, this, &ParameterManager::enablePrimeTowerChanged);
                connect(profile, &PrintProfile::currentProcessPrimeVolumeChanged, this, &ParameterManager::currentProcessPrimeVolumeChanged);
                connect(profile, &PrintProfile::currentProcessPrimeTowerWidthChanged, this, &ParameterManager::currentProcessPrimeTowerWidthChanged);
                connect(profile, &PrintProfile::currentProcessLayerHeightChanged, this, &ParameterManager::currentProcessLayerHeightChanged);
                connect(profile, &PrintProfile::enableSupportChanged, this, &ParameterManager::enableSupportChanged);
                connect(profile, &PrintProfile::currentProcessSupportFilamentChanged, this, &ParameterManager::currentProcessSupportFilamentChanged);
                connect(profile, &PrintProfile::currentProcessSupportInterfaceFilamentChanged, this, &ParameterManager::currentProcessSupportInterfaceFilamentChanged);
                connect(profile, &PrintProfile::printSequenceChanged, this, &ParameterManager::printSequenceChanged);
            }

            onParameterEdited();
        }

        emit currentColorsChanged();
    }

    void ParameterManager::reinitialize() {
        m_addPrinterModel->reinitialize();
        m_machineDatas.clear();
        loadMachineMeta(m_machineDatas);
        loadMachineMeta(m_machineDatas, true);
        loadOfficialMachineNames(m_factoryMachineNames);
        loadUserMachineNames(m_userMachineNames);
        m_materialCenter->reinitialize();
    }

    int ParameterManager::curMachineIndex()
    {
        int index = m_machines.indexOf(m_currentMachine);
        return m_machines.indexOf(m_currentMachine);
    }

    int ParameterManager::functionType()
    {
        return m_functionType;
    }

    int ParameterManager::machinesCount()
    {
        return m_machines.count();
    }

    QStringList ParameterManager::machinesNames()
    {
        return _machineNames(m_machines);
    }

    void ParameterManager::setFunctionType(int type)
    {
        // check
        m_functionType = type;
        cxkernel::setIOFilterKey("base");
        if (m_functionType == 1)
            cxkernel::setIOFilterKey("laser");
        if (m_functionType == 2)
            cxkernel::setIOFilterKey("plotter");
        emit functionTypeChanged();
    }

    void ParameterManager::addMachine(const QString &modelMachineName, const QString &nozzle)
    {
        _addMachineFromUniqueName(_uniqueNameFromBaseNameNozzle(modelMachineName, nozzle));
    }

    QStringList ParameterManager::materialTemplates()
    {
        return fetchFileNames(_pfpt_default_root + "/material_templates", true);
    }

    QStringList ParameterManager::processTemplates()
    {
        return fetchFileNames(_pfpt_default_root + "/process_templates", true);
    }

    QObject* ParameterManager::userMachineModel()
    {
        if (!m_userMachineModel)
        {
            m_userMachineModel = new UserMachineModel(this);
        }
        return m_userMachineModel;
    }

    void ParameterManager::abandonMachine()
    {
        for (auto machine : m_machines)
        {
            if (machine->isModified())
            {
                machine->reset();
            }
        }
    }

    void ParameterManager::saveMachine()
    {
        for (auto machine : m_machines)
        {
            if (machine->isModified())
            {
                machine->mergeAndSaveExtruders();
                machine->mergeAndSave();
                machine->exportSetting(defaultMachineFile(machine->uniqueName(), !machine->isDefault()));
            }
        }
    }

    int ParameterManager::saveMachineAs(const QString& modelMachineName, const QString& userMachineName)
    {
        int machineIndex = -1;
        MachineData data;

        for (const MachineData& machineMeta : m_machineDatas)
        {
            if (machineMeta.uniqueName() == modelMachineName)
            {
                data = machineMeta;
                data.baseName = userMachineName;
                data.codeName = userMachineName;
                data.isUser = true;
            }
        }

        //QString machineName = m_userMachineModel->name();
        PrintMachine* srcMachine = qobject_cast<PrintMachine*>(findMachine(modelMachineName));
        if (!srcMachine)
        {
            return machineIndex;
        }
        QString srcPath, dstPath;
        QString srcMachineUniqueName = srcMachine->uniqueName();
        if (srcMachine->isDefault())
        {
            srcPath = QString("%1/%2/").arg(_pfpt_default_parampack).arg(srcMachineUniqueName);
        }
        else
        {
            srcPath = QString("%1/%2/").arg(_pfpt_user_parampack).arg(srcMachineUniqueName);
        }
        dstPath = QString("%1/%2/").arg(_pfpt_user_parampack).arg(data.uniqueName());

        for (const auto& machine : m_machines)
        {
            if (machine->isUserMachine())
            {
                if (machine->name() == userMachineName || machine->uniqueName() == userMachineName || machine->baseName() == userMachineName)
                {
                    _removeMachine(machine);
                }
            }
        }
        mkMutiDir(dstPath);
        if (!srcMachine->isDefault())
        {
            qtuser_core::copyDir(srcPath, dstPath, true);
        }
        QFile machineFile(QString("%1/%2.def.json").arg(dstPath).arg(srcMachineUniqueName));
        if (machineFile.exists())
            machineFile.remove();
        QFile machineCoverFile(QString("%1/%2.def.json.cover").arg(dstPath).arg(srcMachineUniqueName));
        if (machineCoverFile.exists())
            machineCoverFile.remove();
        srcMachine->exportSetting(QString("%1/%2.def.json").arg(dstPath).arg(data.uniqueName()));
        bool res = addMachineMeta(data);
        // 3.调用原来的添加设备的流程
        srcMachine->reset();
        srcMachine->resetExtruders();
        PrintMachine* machine = new PrintMachine(nullptr, data);
        _addMachine(machine);
        machine->filterMeterialsFromTypes();
        machinesCountChanged();
        machineIndex = m_machines.indexOf(machine);
        return machineIndex;
    }

    QString ParameterManager::modifiedMachineName()
    {
        for (auto machine : m_machines)
        {
            if (machine->isModified())
            {
                return machine->uniqueName();
            }
        }
        return {};
    }

    QString ParameterManager::modifiedMachineFullName()
    {
        for (auto machine : m_machines)
        {
            if (machine->isModified())
            {
                QString res = machine->uniqueName();
                return machine->uniqueName() + " nozzle";
            }
        }
        return {};
    }

    bool ParameterManager::isMachineModified()
    {
        for (auto machine : m_machines)
        {
            if (machine->isModified())
            {
                return true;
            }
        }
        return false;
    }

    void ParameterManager::reloadAffectedMachines(QObject* machineTemplate, const int& modifiedType, const QString& modifiedName)
    {
        PrintMachine* machine_src = qobject_cast<PrintMachine*>(machineTemplate);
        if (!machine_src)
        {
            return;
        }
        for (auto machine : m_machines)
        {
            if (machine == machine_src)
            {
                continue;
            }
            if (machine->inheritsFrom() == machine_src->inheritsFrom())
            {
                switch (modifiedType)
                {
                case 0:
                {
                    machine->reloadMaterial(modifiedName);
                }
                break;
                case 1:
                {
                    machine->reloadProcess(modifiedName);
                }
                break;
                default:
                    break;
                }
            }
        }
    }

    void ParameterManager::addUserMachine(QObject* machineTemplate, const QStringList& materials, const QStringList& processes)
    {
        PrintMachine* machine_src = qobject_cast<PrintMachine*>(machineTemplate);
        if (!machine_src)
        {
            return;
        }
        // 1.读取模板信息到meta
        if (!m_userMachineModel)
            return;

        MachineData data;
        QString machineName = m_userMachineModel->name();

        for (const MachineData& machineMeta : m_machineDatas)
        {
            if (machineMeta.codeName == machineName)
            {
                qWarning() << QString("ParameterManager::_checkUniqueName error. [%1]").arg(machineName);
                return;
            }
        }
        QStringList dirs = {"Materials","Overrides","Processes"};
        float extruder_diameter = m_userMachineModel->nozzleSize();
        float material_diameter = m_userMachineModel->materialDiameter();


        data.baseName = machineName;
        data.codeName = machineName;
        data.extruderCount = 1;
        data.supportMaterialDiameters << material_diameter;
        data.supportMaterialNames = materials;
        data.extruderDiameters << extruder_diameter;
        data.isUser = true;

        QString uniqueName = QString("%1/%2-%3/").arg(_pfpt_user_parampack).arg(machineName).arg(extruder_diameter);
        for (const auto& dir : dirs)
        {
            mkMutiDir(uniqueName + dir);
        }

        QDir material_template_dir(QString("%1/%2/Materials/").arg(_pfpt_default_parampack).arg(machine_src->uniqueName()));
        for (const auto& entry : material_template_dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot)) {
            QFile template_file(entry.absoluteFilePath());
            QString material_name = entry.completeBaseName().remove("-1.75");
            if (materials.contains(material_name))
            {
                QString dstFile = QString("%1%2/%3-%4.json").arg(uniqueName).arg("Materials").arg(material_name).arg(material_diameter);
                template_file.copy(dstFile);
            }
        }

        QDir process_template_dir(QString("%1/%2/Processes/").arg(_pfpt_default_parampack).arg(machine_src->uniqueName()));
        for (const auto& entry : process_template_dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot)) {
            QFile template_file(entry.absoluteFilePath());
            QString process_name = entry.completeBaseName();
            if (processes.contains(process_name))
            {
                QString dstFile = QString("%1%2/%3.json").arg(uniqueName).arg("Processes").arg(process_name);
                template_file.copy(dstFile);
            }
        }

        auto document = rapidjson::Document{ rapidjson::Type::kObjectType };
        auto& allocator = document.GetAllocator();
        auto engine_data_object = rapidjson::Value{ rapidjson::Type::kObjectType };
        std::string printable_area = QString("0x0,%1x0,%1x%2,0x%2").arg(m_userMachineModel->width()).arg(m_userMachineModel->depth()).toStdString();
        engine_data_object.AddMember("printable_area", rapidjson::Value(printable_area.c_str(), allocator), allocator);
        engine_data_object.AddMember("printable_height", rapidjson::Value(std::to_string(m_userMachineModel->height()).c_str(), allocator), allocator);
        engine_data_object.AddMember("machine_gcode_flavor", rapidjson::Value(m_userMachineModel->gcodeFlavor().toStdString().c_str(), allocator), allocator);
        engine_data_object.AddMember("machine_start_gcode", rapidjson::Value(m_userMachineModel->startGcode().toStdString().c_str(), allocator), allocator);
        engine_data_object.AddMember("machine_end_gcode", rapidjson::Value(m_userMachineModel->endGcode().toStdString().c_str(), allocator), allocator);

        document.AddMember("engine_data", std::move(engine_data_object), allocator);
        auto extruder_array = rapidjson::Value{ rapidjson::Type::kArrayType };
        auto extruder_object = rapidjson::Value{ rapidjson::Type::kObjectType };
        auto extruder_engine_data_object = rapidjson::Value{ rapidjson::Type::kObjectType };
        std::string extruder_offset = QString("%1x%2").arg(m_userMachineModel->nozzleOffsetX()).arg(m_userMachineModel->nozzleOffsetY()).toStdString();
        extruder_engine_data_object.AddMember("extruder_offset", rapidjson::Value(extruder_offset.c_str(), allocator), allocator);
        extruder_engine_data_object.AddMember("max_layer_height", rapidjson::Value(std::to_string(m_userMachineModel->nozzleSize()).c_str(), allocator), allocator);
        extruder_engine_data_object.AddMember("nozzle_diameter", rapidjson::Value(std::to_string(m_userMachineModel->nozzleSize()).c_str(), allocator), allocator);
        extruder_object.AddMember("engine_data", std::move(extruder_engine_data_object), allocator);
        extruder_array.PushBack(std::move(extruder_object), allocator);
        document.AddMember("extruders", std::move(extruder_array), allocator);

        rapidjson::StringBuffer strbuf;
        rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
        document.Accept(writer);
        auto filename = QStringLiteral("%1%2-%3.def.json").arg(uniqueName).arg(machineName).arg(extruder_diameter);
        QFile file(filename);
        if (!file.open(QIODevice::WriteOnly)) return;
        std::string json(strbuf.GetString(), strbuf.GetSize());

        file.write(json.c_str(), json.length());
        file.close();

        bool res = addMachineMeta(data);
        // 3.调用原来的添加设备的流程
        PrintMachine *machine = new PrintMachine(nullptr, data);
        _addMachine(machine);
        machine->filterMeterialsFromTypes();
        machinesCountChanged();
    }

    void ParameterManager::importUserMachine(const QString &srcFilePath, const QString &userMachineName, const QString &machineBaseName, const QString &nozzle)
    {
    }

    bool ParameterManager::isMachineInFactory(const QString& name)
    {
        for (const auto& machine : m_machines)
        {
            if (machine->isDefault())
            {
                if (machine->name() == name || machine->uniqueName() == name || machine->baseName() == name)
                {
                    return true;
                }
            }
        }
        return false;
    }

    bool ParameterManager::isMachineInUser(const QString& name)
    {
        for (const auto& machine : m_machines)
        {
            if (machine->isUserMachine())
            {
                if (machine->name() == name || machine->uniqueName() == name || machine->baseName() == name)
                {
                    return true;
                }
            }
        }
        return false;
    }

    bool ParameterManager::addMachineMeta(const MachineData &machineMeta)
    {
        m_machineDatas.append(machineMeta);
        saveMachineMetaUser(machineMeta);
        return true;
    }

    PrintMachine* ParameterManager::currentMachine_s()
    {
        return s_currentMachine;
    }

    bool ParameterManager::isMachineNameRepeat(const QString machineName)
    {
        bool isRepeat = false;
        for (auto data : m_machines)
        {
            if (data->uniqueName() == machineName)
            {
                isRepeat = true;
                break;
            }
        }
        return isRepeat;
    }

    bool ParameterManager::checkMachineExist(const QString &baseName, const QString &nozzle)
    {
        return _checkMachineExist(_uniqueNameFromBaseNameNozzle(baseName, nozzle));
    }

    void ParameterManager::setCurrentMachine(int index, bool needEmit)
    {
        if (index < 0 || index >= m_machines.count())
        {
            qWarning() << QString("ParameterManager::removeMachine error index [%1].").arg(index);
            return;
        }
        auto new_machine = m_machines.at(index);
        //transfer filament
        if (m_currentMachine)
        {
            int old_extruder_count = m_currentMachine->extruderAllCount();
            int new_extruder_count = new_machine->extruderAllCount();
            while (new_extruder_count > old_extruder_count)
            {
                new_machine->removeExtruder();
                --new_extruder_count;
            }
            for (int i = 0; i < m_currentMachine->extruderAllCount(); ++i)
            {
                auto extruder = m_currentMachine->extruders()[i];
                if (new_extruder_count <= i)
                {
                    new_machine->addExtruder();
                }
                if (new_machine->extruders().size() == i)//add failed
                {
                    break;
                }
                auto new_extruder = new_machine->extruders()[i];
                new_extruder->setColor(extruder->color());
                new_extruder->setMaterial(extruder->materialName());
            }
        }
        _setCurrentMachine(new_machine, needEmit);
    }

    void ParameterManager::removeMachine(QObject *machineObject)
    {
        PrintMachine *machine = qobject_cast<PrintMachine *>(machineObject);
        _removeMachine(machine);
    }

    PrintMachine* ParameterManager::machine(int index) {
        if (index >= 0 && index < m_machines.count())
            return m_machines.at(index);

        return m_emptyMachine;
    }

    QObject *ParameterManager::machineObject(int index)
    {
        return machine(index);
    }

    QObject* ParameterManager::machineObject(const QString& machineName)
    {
        for (auto machine : m_machines)
        {
            if (machine->name() == machineName)
            {
                return machine;
            }
        }
        return nullptr;
    }

    QObject *ParameterManager::getMachineObjectByName(QString machineName, float extruderDiameters)
    {
        for (auto data : m_machines)
        {
            if (data->codeName() == machineName && data->machineExtruderDiameters() == extruderDiameters)
            {
                return data;
            }
        }

        return m_emptyMachine;
    }

    QObject* ParameterManager::currentMachineObject()
    {
        PrintMachine* pm = currentMachine();

        return pm;
    }

    PrintMachine* ParameterManager::currentMachine() const {
        return m_currentMachine;
    }

    QString ParameterManager::currentMachineName()
    {
        return m_currentMachine ? m_currentMachine->uniqueName() : QString("INVALID MACHINE");
    }
    QString ParameterManager::currentMachineShowName()
    {
        return m_currentMachine ? m_currentMachine->uniqueShowName() : QString("INVALID MACHINE");
    }

    void ParameterManager::applyCurrentMachine()
    {
        if (m_currentMachine)
        {
            applyPrintMachine(m_currentMachine);
        }
    }

    bool ParameterManager::currentMachineIsBelt()
    {
        return m_currentMachine ? m_currentMachine->isBelt() : false;
    }

    float ParameterManager::currentMachineWidth()
    {
        return m_currentMachine ? m_currentMachine->machineWidth() : 0.0f;
    }
    float ParameterManager::currentMachinedepth()
    {
        return m_currentMachine ? m_currentMachine->machineDepth() : 0.0f;
    }
    float ParameterManager::currentMachineheight()
    {
        return m_currentMachine ? m_currentMachine->machineHeight() : 0.0f;
    }
    float ParameterManager::extruder_clearance_radius()
    {
        return m_currentMachine ? m_currentMachine->extruderClearanceRadius() : 0.0f;
    }
    QString ParameterManager::printSequence()
    {
        return m_currentMachine ? m_currentMachine->_printSequence() : "by layer"; // createCurrentGlobalSettings()->value("print_sequence", "");
    }
    int ParameterManager::currBedTypeIndex()
    {
        int index = currrentBedKeys().indexOf(currBedType());
        qDebug() << currBedType();
        return index;
    }
    QString ParameterManager::currBedType()
    {
        return createCurrentGlobalSettings()->value("curr_bed_type", "");
    }

    QString ParameterManager::firstLayerPrintSequence()
    {
        return createCurrentGlobalSettings()->value("first_layer_print_sequence", "");
    }

    int ParameterManager::currentPlateIndex()
    {
        return m_currentMachine ? m_currentMachine->currentPlateIndex() : 0;
    }

    void ParameterManager::setCurrentPlateIndex(const int& index)
    {
        if (m_currentMachine)
        {
            m_currentMachine->setCurrentPlateIndex(index);
        }
    }

    void ParameterManager::setPrintSequence(const QString& print_sequence)
    {
        if (m_currentMachine)
        {
            m_currentMachine->setPrintSequence(print_sequence);
        }
    }

    void ParameterManager::setCurrBedType(const QString& curr_bed_type)
    {
        if (m_currentMachine)
        {
            m_currentMachine->setCurrBedType(curr_bed_type);
        }
    }

    void ParameterManager::setFirstLayerPrintSequence(const QString& first_layer_print_sequence)
    {
        if (m_currentMachine)
        {
            m_currentMachine->setFirstLayerPrintSequence(first_layer_print_sequence);
        }
    }

    void ParameterManager::resetPlateSettings()
    {
        if (m_currentMachine)
        {
            m_currentMachine->resetPlateSettings();
        }
    }

    bool ParameterManager::enablePrimeTower()
    {
        return m_currentMachine ? m_currentMachine->enablePrimeTower() : false;
    }

    float ParameterManager::currentProcessLayerHeight()
    {
        return m_currentMachine ? m_currentMachine->currentProcessLayerHeight() : 0.0f;
    }

    float ParameterManager::currentProcessPrimeVolume()
    {
        return m_currentMachine ? m_currentMachine->currentProcessPrimeVolume() : 0.0f;
    }

    float ParameterManager::currentProcessPrimeTowerWidth()
    {
        return m_currentMachine ? m_currentMachine->currentProcessPrimeTowerWidth() : 0.0f;
    }

    bool ParameterManager::enableSupport()
    {
        return m_currentMachine ? m_currentMachine->enableSupport() : false;
    }

    int ParameterManager::currentProcessSupportFilament()
    {
        return m_currentMachine ? m_currentMachine->currentProcessSupportFilament() : 0;
    }

    int ParameterManager::currentProcessSupportInterfaceFilament()
    {
        return m_currentMachine ? m_currentMachine->currentProcessSupportInterfaceFilament() : 0;
    }

    float ParameterManager::extruderClearanceRadius()
    {
        return m_currentMachine ? m_currentMachine->extruderClearanceRadius() : 0;
    }

    float ParameterManager::extruderClearanceHeightToLid()
    {
        return m_currentMachine ? m_currentMachine->extruderClearanceHeightToLid() : 0;
    }

    float ParameterManager::extruderClearanceHeightToRod()
    {
        return m_currentMachine ? m_currentMachine->extruderClearanceHeightToRod() : 0;
    }

    std::vector<trimesh::vec3> ParameterManager::extruderClearanceContour()
    {
        std::vector<trimesh::vec3> contour;
        float radius = extruderClearanceRadius() / 2;
        if (radius > 0.0f)
        {
            trimesh::vec3 center = trimesh::vec3(0.0f);
            int iter = 50;
            for (int i = 0; i < iter; i++)
            {
                float pcs = M_2PIf * i / iter;
                contour.emplace_back(trimesh::vec3(cos(pcs) * radius, sin(pcs) * radius, 0.0f));
            }
        }
        return contour;
    }

    bool ParameterManager::checkUniqueName(const QString& uniqueName)
    {
        return true;
    }

    QStringList ParameterManager::printDiffKeys(int printerIndex1, int printerIndex2)
    {
        if (printerIndex1 < 0 || printerIndex2 < 0)
            return QStringList();

        QStringList printList = machineNameList();
        if (printList.count() <= 0)
            return QStringList();

        return printDiffKeys(printList.at(printerIndex1), printList.at(printerIndex2));
    }

    QStringList ParameterManager::printDiffKeys(const QString printerName1, const QString printerName2)
    {
        if (printerName1 == printerName2 || printerName1.isEmpty() || printerName2.isEmpty())
            return QStringList();

        PrintMachine *printer1 = nullptr;
        PrintMachine *printer2 = nullptr;
        for (auto printer : m_machines)
        {
            if (printer->name() == printerName1)
                printer1 = printer;
            else if (printer->name() == printerName2)
                printer2 = printer;
        }

        QStringList diffKeys = printer1->compareSettings(printer2);
        return diffKeys;
    }

    CusAddPrinterModel* ParameterManager::getAddPrinterListModel() const {
        return m_addPrinterModel;
    }

    QObject* ParameterManager::getAddPrinterListModelObject() const {
        return getAddPrinterListModel();
    }

    MaterialCenter* ParameterManager::getMaterialCenter() const {
        return m_materialCenter;
    }

    QObject* ParameterManager::getMaterialCenterObject() const {
        return getMaterialCenter();
    }

    void ParameterManager:: onCheckUpdateableFinished(bool updateable, bool overrideable) {
        if (updateable) {
            requestUpdateableMessage();
        }
    }

    void ParameterManager::requestUpdateableMessage() {
      getKernelUI()->requestMessage(new ParameterUpdateableMessage{ [this]() mutable {
        getKernelUI()->requestQmlDialog(this, QStringLiteral("parameter_update_dialog"));
      } });
    }

    void ParameterManager::onUpdateOfficalPrinterFinished(const QString& uid, bool successed) {
        if (!successed) {
            return;
        }

        if (uid.isEmpty()) {
            getKernelUI()->destroyMessage(MessageType::ParameterUpdateable);
            return;
        }

        for (auto machine : m_machines) {
            auto uidd = machine->uniqueName();
            if (machine->uniqueName() == uid) {
                machine->reload();
                curMachineIndexChanged();
            }
        }
    }

    void ParameterManager::onOverrideOfficalProfileFinished(bool successed) {
        if (successed) {
            reinitialize();
        }
    }

    void ParameterManager::onUpdateProfileLanguageFinished(bool successed) {
        if (successed) {
            m_addPrinterModel->reinitialize();
        }
    }

    void ParameterManager::fileToMachineAndExtruder(const QString &sourceFilePath, const QString &machinePath, const QString &extruderPath)
    {
        QFile file(sourceFilePath);
        QString printerStr;
        QString extruderStr;
        QString &str = printerStr;

        int scope = 0; // 0: machine 1. extruder
        if (file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            while (!file.atEnd())
            {
                QString line = file.readLine().trimmed();
                if (line == "[machine]")
                {
                    scope = 0;
                    continue;
                }
                else if (line == "[Extruder_0]" || line == "[Extruder_1]")
                {
                    scope = 1;
                    continue;
                }
                if (scope == 0)
                {
                    printerStr += line + "\n";
                }
                else
                {
                    extruderStr += line + "\n";
                }
            }
        }
        file.close();

        QFile machineFile(machinePath);
        if (machineFile.exists())
        {
            machineFile.remove();
        }

        if (machineFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
        {
            machineFile.write(printerStr.toUtf8());
        }
        machineFile.close();

        QFile extruderFile(extruderPath);
        if (extruderFile.exists())
        {
            extruderFile.remove();
        }

        if (extruderFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
        {
            extruderFile.write(extruderStr.toUtf8());
        }
        extruderFile.close();
    }

    bool ParameterManager::currentMachineCenterIsZero()
    {
        return m_currentMachine ? /*m_currentMachine->isBelt() || */ m_currentMachine->centerIsZero() : false;
    }

    int ParameterManager::curMachineType()
    {
        return m_currentMachine ? m_currentMachine->machineType() : 1;
    }

    bool ParameterManager::isAnyModified()
    {
        return isMachineModified() || currentMachine()->isCurrentProcessModified() || currentMachine()->isMaterialModified();
    }

    void ParameterManager::abandonAll()
    {
        currentMachine()->abandonMaterialMod();
        currentMachine()->abandonProcessMod();
        abandonMachine();
    }

    int ParameterManager::curExtruderCount()
    {
        if (!m_currentMachine)
            return 1;

        if (m_currentMachine->extruderCount() == 1)
        {
            return m_currentMachine->extruderAllCount();
        }
        else if (m_currentMachine->extruderCount() == 2)
        {
            return 2;
        }
    }

    QList<QColor> ParameterManager::curExtrudersColor()
    {
        PrintMachine* machine = currentMachine();
        int extruderCount = machine->extruderCount();
        QList<QColor> res;
        for (int index = 0; index < extruderCount; ++index)
        {
            QColor color = machine->extruderColor(index);
            res << color;
        }

        return res;
    }

    bool ParameterManager::currentMachineSupportExportImage()
    {
        if (!m_currentMachine)
            return false;

        return m_currentMachine->currentMachineSupportExportImage();
    }

    bool ParameterManager::currentMachineIsBbl()
    {
        return m_currentMachine ? m_currentMachine->currentMachineIsBbl() : false;
    }

    QObject *ParameterManager::findMachine(const QString &machineName)
    {
        QString tempMachineName = machineName;
        QString nozzleStr = " nozzle";
        tempMachineName.remove(nozzleStr);

        for (auto machine : m_machines)
        {
            auto unique_name = machine->uniqueName();
            if (tempMachineName == machine->uniqueName() || tempMachineName == machine->uniqueShowName())
            {
                return machine;
            }
        }
        return nullptr;
    }

    QStringList ParameterManager::extruderSupportDiameters(const QString &machineBaseName, int index)
    {
        QStringList diameters;
        QString basename;
        for (const MachineData &data : m_machineDatas)
        {
            int idx = machineBaseName.indexOf("Creality");
            if (idx == 0)
            {
                basename = machineBaseName.mid(9);
            }
            else
            {
                basename = machineBaseName;
            }
            int res = QString::compare(data.codeName, basename, Qt::CaseInsensitive);
            if (res == 0)
            {
                if (index >= 0 && index < data.extruderCount)
                {
                    diameters.append(QString("%1").arg(data.extruderDiameters[index]));
                    //break;
                }
            }
        }
        return diameters;
    }

    Q_INVOKABLE QString creative_kernel::ParameterManager::getFileNameByPath(const QString &path)
    {
        // 1.判断文件是否存在
        QUrl url(path);
        QFileInfo info(url.toLocalFile());
        QString baseName = info.baseName();
        return baseName;
    }

    Q_INVOKABLE QString creative_kernel::ParameterManager::getMachineName(const QString &path)
    {
        // 1.判断文件是否存在
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
        // 2.获取文件内部保存的机器名称
        QString materialType = "";
        while (!materialFile.atEnd())
        {
            QByteArray line = materialFile.readLine();
            QString qLine(line);
            qLine = qLine.trimmed();
            if (qLine.contains("machine_name"))
            {
                materialType = qLine.mid(QString("machine_name=").length());
            }
        }
        materialFile.close();
        return materialType;
    }
    QString  ParameterManager::getMachineCodeName(const QString& machineName)
    {
        QString codeName = "";
        for (const MachineData& machineData : m_machineDatas)
        {
            if (machineData.baseName == machineName)
            {
                codeName = machineData.codeName;
                break;
            }
        }
        return codeName;
    }


    QString ParameterManager::getMachineNameFromCode(const QString& codeName)
    {
        QString baseName = codeName;
        for (const MachineData& machineData : m_machineDatas)
        {
            if (codeName == machineData.codeName)
            {
                baseName = machineData.baseName;
                break;
            }
        }
        return baseName;
    }

    QString ParameterManager::getUniqueNameFromCode(const QString& codeName)
    {
        QString baseName = codeName;
        for (const MachineData& machineData : m_machineDatas)
        {
            if (codeName == machineData.codeName)
            {
                baseName = machineData.uniqueName();
                break;
            }
            else if (codeName == machineData.baseName)
            {
                baseName = machineData.uniqueName();
                break;
            }
        }
        return baseName;
    }


    int ParameterManager::getMachineExtruderCount(const QString &path)
    {
        // 1.判断文件是否存在
        QUrl url(path);
        QFile materialFile(url.toLocalFile());
        if (!materialFile.exists())
        {
            qDebug() << "文件不存在";
            return 0;
        }
        if (!materialFile.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            return 0;
        }
        // 2.获取文件内部保存的机器名称
        QString materialType = "";
        while (!materialFile.atEnd())
        {
            QByteArray line = materialFile.readLine();
            QString qLine(line);
            qLine = qLine.trimmed();
            if (qLine.contains("machine_extruder_count"))
            {
                materialType = qLine.mid(QString("machine_extruder_count=").length());
            }
        }
        materialFile.close();
        return materialType.toInt();
    }
    void ParameterManager::importPrinterFromOrca(MachineData &meta)
    {
        m_machineDatas.append(meta);
        saveMachineMetaUser(meta);
        QMetaObject::invokeMethod(this, [this, meta]() {
            PrintMachine* machine = new PrintMachine(nullptr, meta);
            _addImportMachine(machine);
            }, Qt::ConnectionType::QueuedConnection);
    }
    void ParameterManager::importPrinterFromQml(const QString &urlString)
    {
        QUrl url(urlString);

        QFileInfo info(url.toLocalFile());
        QString baseName = info.baseName();

        QString profileName = baseName;

        //
        PrintMachine *machine = _createMachine(urlString);
        if (machine)
        {
            machine->setParent(this);
            machine->added();
            if(machine->isDefault())
            {
                m_machines.push_back(machine);
            }else{
                m_machines.push_front(machine);
            }
            m_MachineListModel->insertMachine(machine, m_machines.indexOf(machine));
        }

        emit machineNameListChanged();
        emit userMachineNameListChanged();
        emit defaultMachineNameListChanged();
        emit machinesCountChanged();
    }

    bool ParameterManager::checkVisSceneSensitiveParam(const QString &key) const
    {
        static const std::set<QString> sensitive_params{
            // 装填塔相关
            QStringLiteral("prime_tower_enable"),
            QStringLiteral("prime_tower_position_x"),
            QStringLiteral("prime_tower_position_y"),
            QStringLiteral("prime_tower_size"),
            // ...
        };

        return sensitive_params.find(key) != sensitive_params.cend();
    }

    void ParameterManager::syncCurParamToVisScene() const
    {
        PrintProfile *profile{nullptr};
        if (m_currentMachine)
        {
            profile = m_currentMachine->profile(m_currentMachine->curProfileIndex());
        }

        static const auto s2b = [](const QString &string) -> bool
        {
            return string == QStringLiteral("true");
        };

        const auto read_setting = [profile](const QString &key) -> QString
        {
            return profile ? profile->userSettings()->value(key,
                                                            profile->settings()->value(key, {}))
                           : QString{};
        };

        // 装填塔相关
        if (profile && s2b(read_setting(QStringLiteral("prime_tower_enable"))))
        {
            const auto brim_enable = s2b(read_setting(QStringLiteral("prime_tower_brim_enable")));
            const auto tower_size = read_setting(QStringLiteral("prime_tower_size")).toFloat() / 2;
            const auto brim_width = read_setting(QStringLiteral("brim_width")).toFloat();
            creative_kernel::showPrimeTower(
                read_setting(QStringLiteral("prime_tower_position_x")).toFloat(),
                read_setting(QStringLiteral("prime_tower_position_y")).toFloat(),
                brim_enable ? tower_size + brim_width : tower_size);
        }
        else
        {
            creative_kernel::hidePrimeTower();
        }
        // ...
    }

    void ParameterManager::saveCurrentProjectSettings(const QString& filename)
    {
        QFile file(filename);
        if (!file.open(QIODevice::WriteOnly))
        {
            return;
        }
        QStringList needEscape = { "gcode_start","inter_layer","gcode_end", "machine_start_gcode" ,"machine_extruder_start_code", "machine_extruder_end_code","machine_end_gcode" };
        PrintMachine* machine = currentMachine();

        auto document = rapidjson::Document{ rapidjson::Type::kObjectType };
        auto& allocator = document.GetAllocator();
        auto metaObject = rapidjson::Value{ rapidjson::Type::kObjectType };

        auto machineObject = rapidjson::Value{ rapidjson::Type::kObjectType };
        auto different_settings_to_factory = rapidjson::Value{ rapidjson::Type::kObjectType };
        for (auto setting : machine->userSettings()->hashSettings())
        {
            QString v = setting->str();
            QString key = setting->key();

            if (needEscape.contains(key))
            {
                v = v.replace("\n", "\\n");
            }
            different_settings_to_factory.AddMember(rapidjson::Value(key.toStdString().c_str(), allocator), rapidjson::Value(v.toStdString().c_str(), allocator), allocator);
        }
        machineObject.AddMember("different_settings_to_factory", std::move(different_settings_to_factory), allocator);
        QString factory_profile = machine->isUserMachine() ? machine->inheritsFrom() : machine->uniqueName();
        machineObject.AddMember("factory_profile", rapidjson::Value(factory_profile.toStdString().c_str(), allocator), allocator);
        machineObject.AddMember("current_profile", rapidjson::Value(machine->uniqueName().toStdString().c_str(), allocator), allocator);
        document.AddMember("machine", std::move(machineObject), allocator);

        auto extruder_array = rapidjson::Value{ rapidjson::Type::kArrayType };
        for (int i = 0; i < curExtruderCount(); i++)
        {
            PrintExtruder* extruder = qobject_cast<PrintExtruder*>(machine->extruderObject(i));
            auto extruder_object = rapidjson::Value{ rapidjson::Type::kObjectType };
            auto different_settings_to_factory = rapidjson::Value{ rapidjson::Type::kObjectType };
            if (extruder->userSettings())
            {
                for (auto setting : extruder->userSettings()->hashSettings())
                {
                    QString v = setting->str();
                    QString key = setting->key();

                    if (needEscape.contains(key))
                    {
                        v = v.replace("\n", "\\n");
                    }
                    different_settings_to_factory.AddMember(rapidjson::Value(key.toStdString().c_str(), allocator), rapidjson::Value(v.toStdString().c_str(), allocator), allocator);
                }
            }
            extruder_object.AddMember("different_settings_to_factory", std::move(different_settings_to_factory), allocator);

            PrintMaterial* material = machine->materialWithName(extruder->materialName());
            auto material_object = rapidjson::Value{ rapidjson::Type::kObjectType };
            different_settings_to_factory = rapidjson::Value{ rapidjson::Type::kObjectType };
            if (extruder->userSettings())
            {
                for (auto setting : extruder->userSettings()->hashSettings())
                {
                    QString v = setting->str();
                    QString key = setting->key();

                    if (needEscape.contains(key))
                    {
                        v = v.replace("\n", "\\n");
                    }
                    different_settings_to_factory.AddMember(rapidjson::Value(key.toStdString().c_str(), allocator), rapidjson::Value(v.toStdString().c_str(), allocator), allocator);
                }
            }
            material_object.AddMember("different_settings_to_factory", std::move(different_settings_to_factory), allocator);
            QString factory_profile;
            if (material)
            {
                factory_profile = material->isUserDef() ? material->inheritsFrom() : material->uniqueName();
            }
            material_object.AddMember("factory_profile", rapidjson::Value(factory_profile.toStdString().c_str(), allocator), allocator);
            material_object.AddMember("current_profile", rapidjson::Value(material ? material->uniqueName().toStdString().c_str() : "", allocator), allocator);
            extruder_object.AddMember("material", std::move(material_object), allocator);
            extruder_object.AddMember("color", extruder->color().rgba64().toArgb32(), allocator);
            extruder_array.PushBack(extruder_object, allocator);
        }
        document.AddMember("extruder", std::move(extruder_array), allocator);

        PrintProfile* profile = machine->currentProfile();
        auto profile_object = rapidjson::Value{ rapidjson::Type::kObjectType };
        different_settings_to_factory = rapidjson::Value{ rapidjson::Type::kObjectType };
        if (profile->userSettings())
        {
            for (auto setting : profile->userSettings()->hashSettings())
            {
                QString v = setting->str();
                QString key = setting->key();

                if (needEscape.contains(key))
                {
                    v = v.replace("\n", "\\n");
                }
                different_settings_to_factory.AddMember(rapidjson::Value(key.toStdString().c_str(), allocator), rapidjson::Value(v.toStdString().c_str(), allocator), allocator);
            }
        }
        if (!profile->isDefault())
        {
            for (auto setting : profile->settings()->hashSettings())
            {
                QString v = setting->str();
                QString key = setting->key();

                if (needEscape.contains(key))
                {
                    v = v.replace("\n", "\\n");
                }
                if (!different_settings_to_factory.HasMember(rapidjson::Value(key.toStdString().c_str(), allocator)))
                {
                    different_settings_to_factory.AddMember(rapidjson::Value(key.toStdString().c_str(), allocator), rapidjson::Value(v.toStdString().c_str(), allocator), allocator);
                }
            }
        }
        profile_object.AddMember("different_settings_to_factory", std::move(different_settings_to_factory), allocator);
        factory_profile = !profile->isDefault() ? profile->inheritsFrom() : profile->uniqueName();
        profile_object.AddMember("factory_profile", rapidjson::Value(factory_profile.toStdString().c_str(), allocator), allocator);
        profile_object.AddMember("current_profile", rapidjson::Value(profile->uniqueName().toStdString().c_str(), allocator), allocator);
        document.AddMember("process", std::move(profile_object), allocator);

        document.AddMember("printer_model", rapidjson::Value(machine->codeName().toStdString().c_str(), allocator), allocator);
        document.AddMember("printer_variant", rapidjson::Value(machine->nozzleDiameter()[0].toStdString().c_str(), allocator), allocator);
        document.AddMember("default_print_profile", rapidjson::Value(profile->uniqueName().toStdString().c_str(), allocator), allocator);
        document.AddMember("printer_settings_id", rapidjson::Value(machine->codeName().toStdString().c_str(), allocator), allocator);
        document.AddMember("print_settings_id", rapidjson::Value(profile->uniqueName().toStdString().c_str(), allocator), allocator);
        auto material_array = rapidjson::Value{ rapidjson::Type::kArrayType };
        auto material_color_array = rapidjson::Value{ rapidjson::Type::kArrayType };
        for (int i = 0; i < curExtruderCount(); i++)
        {
            PrintExtruder* extruder = qobject_cast<PrintExtruder*>(machine->extruderObject(i));
            if (!extruder)
            {
                continue;
            }
            PrintMaterial* material = machine->materialWithName(extruder->materialName());
            if (!material)
            {
                continue;
            }
            material_array.PushBack(rapidjson::Value(material->name().toStdString().c_str(), allocator), allocator);
            material_color_array.PushBack(rapidjson::Value(extruder->color().name().toStdString().c_str(), allocator), allocator);
        }
        document.AddMember("filament_settings_id", std::move(material_array), allocator);
        document.AddMember("filament_colour", std::move(material_color_array), allocator);

        QStringList machine_parameter_keys = getMetaMachineKeys();
        QStringList extruder_parameter_keys = getMetaExtruderKeys();
        QStringList profile_parameter_keys = getMetaProfileKeys();
        QStringList material_parameter_keys = getMetaMaterialKeys();
        auto funcAddString = [&needEscape, &document, &allocator](ParameterBase* machine, QStringList machine_parameter_keys) {
            for (const auto& key : machine_parameter_keys)
            {
                QString v;
                if (machine->userSettings()->hasKey(key))
                {
                    v = machine->userSettings()->value(key, "");
                }
                else if (machine->settings()->hasKey(key))
                {
                    v = machine->settings()->value(key, "");
                }
                else
                {
                    continue;
                }

                if (needEscape.contains(key))
                {
                    v = v.replace("\n", "\\n");
                }
                if (!document.HasMember(key.toStdString().c_str()))
                {
                    document.AddMember(rapidjson::Value(key.toStdString().c_str(), allocator), rapidjson::Value(v.toStdString().c_str(), allocator), allocator);
                }
            }
        };
        funcAddString(machine, machine_parameter_keys);
        PrintExtruder* extruder = qobject_cast<PrintExtruder*>(machine->extruderObject(0));
        funcAddString(extruder, extruder_parameter_keys);
        funcAddString(profile, profile_parameter_keys);

        for (const auto& key : material_parameter_keys)
        {
            auto extruder_array = rapidjson::Value{ rapidjson::Type::kArrayType };
            for (int i = 0; i < curExtruderCount(); i++)
            {
                PrintExtruder* extruder = qobject_cast<PrintExtruder*>(machine->extruderObject(i));
                if (!extruder)
                {
                    continue;
                }
                PrintMaterial* material = machine->materialWithName(extruder->materialName());
                if (!material)
                {
                    continue;
                }
                QString v;
                if (material->userSettings()->hasKey(key))
                {
                    v = material->userSettings()->value(key, "");
                }
                else if (material->settings()->hasKey(key))
                {
                    v = material->settings()->value(key, "");
                }
                else
                {
                    continue;
                }

                if (needEscape.contains(key))
                {
                    v = v.replace("\n", "\\n");
                }
                extruder_array.PushBack(rapidjson::Value(v.toStdString().c_str(), allocator), allocator);
            }
            if (!document.HasMember(key.toStdString().c_str()))
            {
                document.AddMember(rapidjson::Value(key.toStdString().c_str(), allocator), std::move(extruder_array), allocator);
            }
        }

        rapidjson::StringBuffer strbuf;
        rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
        document.Accept(writer);
        std::string json(strbuf.GetString(), strbuf.GetSize());
        file.write(json.c_str(), json.length());
        file.close();
    }
    QMap<int, int> ParameterManager::loadModelsSettings(const QString& filename)
    {
        QMap<int, int> modelExters;
        if (!QFile(filename).exists())
        {
            return modelExters;
        }

        QDomDocument doc("mydocument");
        QFile file(filename);
        if (!file.open(QIODevice::ReadOnly))
            return modelExters;
        if (!doc.setContent(&file))
        {
            file.close();
            return modelExters;
        }
        file.close();
        QDomElement docElem = doc.documentElement();
        QDomNode n = docElem.firstChild();
        while (!n.isNull())
        {
            QDomElement e = n.toElement();
            qDebug() << e.tagName();
            if (!e.isNull())
            {
                if (e.tagName() == "object")
                {
                    QDomNode part = e.firstChild();
                    int objectId = e.attribute("id").toInt();
                    while (!part.isNull())
                    {
                        QDomElement p = part.toElement();
                        if (p.tagName() == "part")
                        {
                            int partId = p.attribute("id").toInt();
                            QDomNode metadata = p.firstChild();

                            while (!metadata.isNull())
                            {
                                QDomElement m = metadata.toElement();
                                if (m.tagName() == "metadata")
                                {
                                   QString key = m.attribute("key");
                                   if (key == "extruder")
                                   {
                                       QString val = m.attribute("value");
                                       modelExters.insert(partId, val.toInt());
                                   }
                                }
                                metadata = metadata.nextSibling();
                            }
                        }

                        qDebug() << e.tagName();
                        if (p.tagName() == "metadata")
                        {
                            QString key = p.attribute("key");
                            if (key == "extruder")
                            {
                                QString val = p.attribute("value");
                                //modelExters.insert(objectId, val.toInt());
                            }
                        }
                        part = part.nextSibling();
                    }
                }
                ; // try to convert the node to an eleme
                //cout << qPrintable(e.tagName()) << endl; // the nodereally is an
                n = n.nextSibling();
            }
         }

        return modelExters;
    }

    void ParameterManager::handleLegacy(std::string& opt_key, std::string& value)
    {
        //BBS: handle legacy options
        if (opt_key == "enable_wipe_tower") {
            opt_key = "enable_prime_tower";
        }
        else if (opt_key == "wipe_tower_width") {
            opt_key = "prime_tower_width";
        }
        else if (opt_key == "bottom_solid_infill_flow_ratio") {
            opt_key = "initial_layer_flow_ratio";
        }
        else if (opt_key == "wiping_volume") {
            opt_key = "prime_volume";
        }
        else if (opt_key == "wipe_tower_brim_width") {
            opt_key = "prime_tower_brim_width";
        }
        else if (opt_key == "tool_change_gcode") {
            opt_key = "change_filament_gcode";
        }
        else if (opt_key == "bridge_fan_speed") {
            opt_key = "overhang_fan_speed";
        }
        else if (opt_key == "infill_extruder") {
            opt_key = "sparse_infill_filament";
        }
        else if (opt_key == "solid_infill_extruder") {
            opt_key = "solid_infill_filament";
        }
        else if (opt_key == "perimeter_extruder") {
            opt_key = "wall_filament";
        }
        else if (opt_key == "support_material_extruder") {
            opt_key = "support_filament";
        }
        else if (opt_key == "support_material_interface_extruder") {
            opt_key = "support_interface_filament";
        }
        else if (opt_key == "support_material_angle") {
            opt_key = "support_angle";
        }
        else if (opt_key == "support_material_enforce_layers") {
            opt_key = "enforce_support_layers";
        }
        else if ((opt_key == "initial_layer_print_height" ||
            opt_key == "initial_layer_speed" ||
            opt_key == "internal_solid_infill_speed" ||
            opt_key == "top_surface_speed" ||
            opt_key == "support_interface_speed" ||
            opt_key == "outer_wall_speed" ||
            opt_key == "support_object_xy_distance") && value.find("%") != std::string::npos) {
            //BBS: this is old profile in which value is expressed as percentage.
            //But now these key-value must be absolute value.
            //Reset to default value by erasing these key to avoid parsing error.
            opt_key = "";
        }
        else if (opt_key == "inherits_cummulative") {
            opt_key = "inherits_group";
        }
        else if (opt_key == "compatible_printers_condition_cummulative") {
            opt_key = "compatible_machine_expression_group";
        }
        else if (opt_key == "compatible_prints_condition_cummulative") {
            opt_key = "compatible_process_expression_group";
        }
        else if (opt_key == "cooling") {
            opt_key = "slow_down_for_layer_cooling";
        }
        else if (opt_key == "timelapse_no_toolhead") {
            opt_key = "timelapse_type";
        }
        else if (opt_key == "timelapse_type" && value == "2") {
            // old file "0" is None, "2" is Traditional
            // new file "0" is Traditional, erase "2"
            value = "0";
        }
        else if (opt_key == "support_type" && value == "normal") {
            value = "normal(manual)";
        }
        else if (opt_key == "support_type" && value == "tree") {
            value = "tree(manual)";
        }
        else if (opt_key == "support_type" && value == "hybrid(auto)") {
            value = "tree(auto)";
        }
        else if (opt_key == "support_base_pattern" && value == "none") {
            value = "hollow";
        }
        else if (opt_key == "infill_anchor") {
            opt_key = "sparse_infill_anchor";
        }
        else if (opt_key == "infill_anchor_max") {
            opt_key = "sparse_infill_anchor_max";
        }
        else if (opt_key == "overhang_fan_threshold" && value == "5%") {
            value = "10%";
        }
        else if (opt_key == "wall_infill_order") {
            if (value == "inner wall/outer wall/infill" || value == "infill/inner wall/outer wall") {
                opt_key = "wall_sequence";
                value = "inner wall/outer wall";
            }
            else if (value == "outer wall/inner wall/infill" || value == "infill/outer wall/inner wall") {
                opt_key = "wall_sequence";
                value = "outer wall/inner wall";
            }
            else if (value == "inner-outer-inner wall/infill") {
                opt_key = "wall_sequence";
                value = "inner-outer-inner wall";
            }
            else {
                opt_key = "wall_sequence";
            }
        }

        // Ignore the following obsolete configuration keys:
        static std::set<std::string> ignore = {
            "acceleration", "scale", "rotate", "duplicate", "duplicate_grid",
            "bed_size",
            "print_center", "g0", "wipe_tower_per_color_wipe"
    #ifndef HAS_PRESSURE_EQUALIZER
            , "max_volumetric_extrusion_rate_slope_positive", "max_volumetric_extrusion_rate_slope_negative"
    #endif /* HAS_PRESSURE_EQUALIZER */
            // BBS
            , "support_sharp_tails","support_remove_small_overhangs", "support_with_sheath",
            "tree_support_branch_diameter_angle", "tree_support_collision_resolution", "tree_support_with_infill",
            "max_volumetric_speed", "max_print_speed",
            "support_closing_radius",
            "remove_freq_sweep", "remove_bed_leveling", "remove_extrusion_calibration",
            "support_transition_line_width", "support_transition_speed", "bed_temperature", "bed_temperature_initial_layer",
            "can_switch_nozzle_type", "can_add_auxiliary_fan", "extra_flush_volume", "spaghetti_detector", "adaptive_layer_height",
            "z_hop_type","nozzle_hrc","chamber_temperature","only_one_wall_top","bed_temperature_difference"
        };

        if (ignore.find(opt_key) != ignore.end()) {
            opt_key = "";
            return;
        }
    }

    void ParameterManager::loadProjectSettings(const QString& filename)
    {
        MachineData data;
        QFile jsonFile(filename);
        if (!jsonFile.exists() || !jsonFile.open(QIODevice::ReadOnly))
        {
            return;
        }
        rapidjson::Document document;
        document.Parse(jsonFile.readAll().constData());
        if (document.HasParseError())
        {
            return;
        }

        auto& alloc = document.GetAllocator();
        for (rapidjson::Value::MemberIterator it = document.MemberBegin(); it != document.MemberEnd(); it++)
        {
            if (it->value.IsString())
            {
                std::string old_key = it->name.GetString();
                std::string key = old_key;
                std::string old_value = it->value.GetString();
                std::string value = old_value;
                handleLegacy(key, value);
                if (key.empty())
                {
                    document.RemoveMember(it->name);
                }
                else
                {
                    if (key != old_key)
                    {
                        it->name = rapidjson::Value(key.c_str(), alloc);
                    }
                    if (value != old_value)
                    {
                        it->value = rapidjson::Value(value.c_str(), alloc);
                    }
                }
            }
        }

        QString printer_model;
        QString printer_variant;
        QString curr_bed_type, print_sequence, first_layer_print_sequence;
        if (document.HasMember("curr_bed_type") && document["curr_bed_type"].IsString())
        {
            curr_bed_type = document["curr_bed_type"].GetString();
        }
        if (document.HasMember("print_sequence") && document["print_sequence"].IsString())
        {
            print_sequence = document["print_sequence"].GetString();
        }
        if (document.HasMember("first_layer_print_sequence") && document["first_layer_print_sequence"].IsString())
        {
            first_layer_print_sequence = document["first_layer_print_sequence"].GetString();
        }
        if (document.HasMember("printer_model") && document["printer_model"].IsString())
        {
            printer_model = document["printer_model"].GetString();
        }
        if (document.HasMember("printer_variant") && document["printer_variant"].IsString())
        {
            printer_variant = document["printer_variant"].GetString();
        }

        data.baseName = printer_model;
        data.codeName = printer_model;
        data.extruderDiameters << printer_variant.toFloat();
        bool machine_exist = false;
        auto machine_unique_name = data.uniqueName();
        QString machine_file_name;
        machine_file_name = defaultMachineFile(data.uniqueName(), true);
        makeSureUserParamPackDir(data.uniqueName());
        for (const auto& machine_data : m_machineDatas)
        {
            if (machine_data.uniqueName() == machine_unique_name)
            {
                machine_exist = true;
                data = machine_data;
                break;
            }
        }
        if(machine_exist && !data.is_import)
        {
            rapidjson::Document docJsonFile;
            auto& allocator = docJsonFile.GetAllocator();
            if (document.HasMember("machine") && document["machine"].IsObject())
            {
                docJsonFile.SetObject();
                auto machine_object = document["machine"].GetObj();
                if (machine_object.HasMember("different_settings_to_factory") && machine_object["different_settings_to_factory"].IsObject())
                {
                    auto engine_object = machine_object["different_settings_to_factory"].GetObj();
                    docJsonFile.AddMember("engine_data", engine_object, allocator);
                    rapidjson::StringBuffer strbuf;
                    rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
                    docJsonFile.Accept(writer);
                    QFile machine_cover_file(machineCoverFile(machine_unique_name));
                    if (!machine_cover_file.open(QIODevice::WriteOnly))
                    {
                        return;
                    }
                    machine_cover_file.write(std::string(strbuf.GetString(), strbuf.GetSize()).c_str(), strbuf.GetSize());
                    machine_cover_file.close();
                }
            }
            removeCacheMachine(data.uniqueName());
            if (document.HasMember("extruder") && document["extruder"].IsArray())
            {
                docJsonFile.SetObject();
                auto extruder_array = document["extruder"].GetArray();
                for (size_t i = 0; i < extruder_array.Size(); i++)
                {
                    docJsonFile.SetObject();
                    auto extruder_object = extruder_array[i].GetObj();

                    const auto& color = extruder_object["color"].GetUint();
                    writeCacheMachineExtruder(machine_unique_name, QColor(color), i ? false : true);
                    if (extruder_object.HasMember("material") && extruder_object["material"].IsObject())
                    {
                        auto material_object = extruder_object["material"].GetObj();
                        QString material_name;
                        if (material_object.HasMember("current_profile") && material_object["current_profile"].IsString())
                        {
                            material_name = material_object["current_profile"].GetString();
                            writeCacheExtruderMaterialIndex(machine_unique_name, i, material_name);
                        }
                        if (material_object.HasMember("different_settings_to_factory") && material_object["different_settings_to_factory"].IsObject())
                        {
                            auto engine_object = material_object["different_settings_to_factory"].GetObj();
                            docJsonFile.AddMember("engine_data", engine_object, allocator);
                            rapidjson::StringBuffer strbuf;
                            rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
                            docJsonFile.Accept(writer);
                            QFile material_cover_file(materialCoverFile(machine_unique_name, material_name));
                            if (!material_cover_file.open(QIODevice::WriteOnly))
                            {
                                return;
                            }
                            material_cover_file.write(std::string(strbuf.GetString(), strbuf.GetSize()).c_str(), strbuf.GetSize());
                            material_cover_file.close();
                        }
                    }
                    if (extruder_object.HasMember("different_settings_to_factory") && extruder_object["different_settings_to_factory"].IsObject())
                    {
                        auto engine_object = extruder_object["different_settings_to_factory"].GetObj();
                        docJsonFile.AddMember("engine_data", engine_object, allocator);
                        rapidjson::StringBuffer strbuf;
                        rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
                        docJsonFile.Accept(writer);
                        QFile extruder_cover_file(userExtruderFile(machine_unique_name, i));
                        if (!extruder_cover_file.open(QIODevice::WriteOnly))
                        {
                            return;
                        }
                        extruder_cover_file.write(std::string(strbuf.GetString(), strbuf.GetSize()).c_str(), strbuf.GetSize());
                        extruder_cover_file.close();
                    }
                }
            }

            if (document.HasMember("process") && document["process"].IsObject())
            {
                docJsonFile.SetObject();
                auto process_object = document["process"].GetObj();
                QString process_name;
                if (process_object.HasMember("current_profile") && process_object["current_profile"].IsString())
                {
                    process_name = process_object["current_profile"].GetString();
                }
                if (process_object.HasMember("different_settings_to_factory") && process_object["different_settings_to_factory"].IsObject())
                {
                    auto engine_object = process_object["different_settings_to_factory"].GetObj();
                    docJsonFile.AddMember("engine_data", engine_object, allocator);
                    rapidjson::StringBuffer strbuf;
                    rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
                    docJsonFile.Accept(writer);
                    QFile process_cover_file(userProfileCoverFile(machine_unique_name, process_name + ".json"));
                    if (!process_cover_file.open(QIODevice::WriteOnly))
                    {
                        return;
                    }
                    process_cover_file.write(std::string(strbuf.GetString(), strbuf.GetSize()).c_str(), strbuf.GetSize());
                    process_cover_file.close();
                }
                writeCacheCurrentProfile(machine_unique_name, process_name);
            }

            _addMachineFromUniqueName(data.uniqueName());
            setCurrBedType(curr_bed_type);
            setPrintSequence(print_sequence);
            setFirstLayerPrintSequence(first_layer_print_sequence);
            return;
        }

        data.isUser = true;
        data.is_import = true;
        rapidjson::Document docJsonFile;
        docJsonFile.SetObject();
        auto& allocator = docJsonFile.GetAllocator();

        QStringList machine_parameter_keys = getMetaMachineKeys();
        QStringList extruder_parameter_keys = getMetaExtruderKeys();
        QStringList profile_parameter_keys = getMetaProfileKeys();
        QStringList material_parameter_keys = getMetaMaterialKeys();

        auto printerObj = rapidjson::Value{ rapidjson::Type::kObjectType };
        for (const auto& key : machine_parameter_keys)
        {
            auto key_string = key.toStdString();
            const char* ckey = key_string.c_str();
            if (document.HasMember(ckey) && document[ckey].IsString())
            {
                printerObj.AddMember(rapidjson::Value(ckey, allocator), rapidjson::Value(document[ckey].GetString(), allocator), allocator);
            }
            if (document.HasMember(ckey) && document[ckey].IsArray())
            {
                const auto arrayObj = document[ckey].GetArray();
                std::string value;
                for (size_t i = 0; i < arrayObj.Size(); i++)
                {
                    if (!value.empty())
                    {
                        value += ",";
                    }
                    value += arrayObj[i].GetString();
                }
                printerObj.AddMember(rapidjson::Value(ckey, allocator), rapidjson::Value(value.c_str(), allocator), allocator);
            }
        }
        //rapidjson::Value v1(document, allocator);
        docJsonFile.AddMember("printer", printerObj, allocator);

        auto extruder_array = rapidjson::Value{ rapidjson::Type::kArrayType };
        for (int i = 0; i < 1; i++)
        {
            auto extruder_object = rapidjson::Value{ rapidjson::Type::kObjectType };
            auto engine_object = rapidjson::Value{ rapidjson::Type::kObjectType };
            for (const auto& key : extruder_parameter_keys)
            {
                auto key_string = key.toStdString();
                const char* ckey = key_string.c_str();
                if (document.HasMember(ckey) && document[ckey].IsString())
                {
                    engine_object.AddMember(rapidjson::Value(ckey, allocator), rapidjson::Value(document[ckey].GetString(), allocator), allocator);
                }
                if (document.HasMember(ckey) && document[ckey].IsArray())
                {
                    const auto& array = document[ckey][0];
                    engine_object.AddMember(rapidjson::Value(ckey, allocator), rapidjson::Value(array.GetString(), allocator), allocator);
                }
            }
            extruder_object.AddMember("engine_data", engine_object, allocator);
            extruder_array.PushBack(extruder_object, allocator);
        }
        docJsonFile.AddMember("extruders", std::move(extruder_array), allocator);
        rapidjson::StringBuffer strbuf;
        rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
        docJsonFile.Accept(writer);
        QFile machine_cover_file(machine_file_name);
        if (!machine_cover_file.open(QIODevice::WriteOnly))
        {
            return;
        }
        machine_cover_file.write(std::string(strbuf.GetString(), strbuf.GetSize()).c_str(), strbuf.GetSize());
        machine_cover_file.close();

        docJsonFile.SetObject();
        auto processObj = rapidjson::Value{ rapidjson::Type::kObjectType };

        for (const auto& key : profile_parameter_keys)
        {
            auto key_string = key.toStdString();
            const char* ckey = key_string.c_str();
            if (document.HasMember(ckey) && document[ckey].IsString())
            {
                processObj.AddMember(rapidjson::Value(ckey, allocator), rapidjson::Value(document[ckey].GetString(), allocator), allocator);
            }
            if (document.HasMember(ckey) && document[ckey].IsArray())
            {
                const auto arrayObj = document[ckey].GetArray();
                std::string value;
                for (size_t i = 0; i < arrayObj.Size(); i++)
                {
                    if (!value.empty())
                    {
                        value += ",";
                    }
                    value += arrayObj[i].GetString();
                }
                processObj.AddMember(rapidjson::Value(ckey, allocator), rapidjson::Value(value.c_str(), allocator), allocator);
            }
        }
        docJsonFile.AddMember("engine_data", std::move(processObj), allocator);
        QString default_print_profile;
        if (document.HasMember("print_settings_id") && document["print_settings_id"].IsString())
        {
            default_print_profile = document["print_settings_id"].GetString();
        }
        QString process_file_name = userProfileFile(data.uniqueName(), default_print_profile);

        rapidjson::StringBuffer strbufProcess;
        rapidjson::Writer<rapidjson::StringBuffer> writerProcess(strbufProcess);
        docJsonFile.Accept(writerProcess);
        QFile process_json_file(process_file_name);
        if (!process_json_file.open(QIODevice::WriteOnly))
        {
            return;
        }
        process_json_file.write(std::string(strbufProcess.GetString(), strbufProcess.GetSize()).c_str(), strbufProcess.GetSize());
        process_json_file.close();

        QStringList print_materials;
        if (document.HasMember("filament_settings_id") && document["filament_settings_id"].IsArray())
        {
            const auto& array = document["filament_settings_id"];
            for (size_t i = 0; i < array.Size(); i++)
            {
                print_materials << QString("%1-1.75").arg(array[i].GetString());
            }
        }
        for (size_t i = 0; i < print_materials.size(); i++)
        {
            docJsonFile.SetObject();
            QString material_file_name = defaultMaterialFile(data.uniqueName(), print_materials[i], true);
            auto engine_object = rapidjson::Value{ rapidjson::Type::kObjectType };
            for (const auto& key : material_parameter_keys)
            {
                auto key_string = key.toStdString();
                const char* ckey = key_string.c_str();
                if (document.HasMember(ckey) && document[ckey].IsArray())
                {
                    const auto& array = document[ckey];
                    if (array.Size() > i)
                    {
                        engine_object.AddMember(rapidjson::Value(ckey, allocator), rapidjson::Value(array[i].GetString(), allocator), allocator);
                    }
                }
            }
            docJsonFile.AddMember("engine_data", std::move(engine_object), allocator);
            rapidjson::StringBuffer strbufProcess;
            rapidjson::Writer<rapidjson::StringBuffer> writerProcess(strbufProcess);
            docJsonFile.Accept(writerProcess);
            QFile material_json_file(material_file_name);
            if (!material_json_file.open(QIODevice::WriteOnly))
            {
                return;
            }
            material_json_file.write(std::string(strbufProcess.GetString(), strbufProcess.GetSize()).c_str(), strbufProcess.GetSize());
            material_json_file.close();
        }

        removeCacheMachine(data.uniqueName());
        if (document.HasMember("filament_colour") && document["filament_colour"].IsArray())
        {
            const auto& array = document["filament_colour"].GetArray();
            for (size_t i = 0; i < array.Size(); i++)
            {
                const auto& color_string = array[i].GetString();
                QColor color(color_string);
                if (strlen(color_string) > 7)
                {
                    auto a = color.alpha();
                    auto r = color.red();
                    auto g = color.green();
                    auto b = color.blue();
                    color.setAlpha(b);
                    color.setRed(a);
                    color.setGreen(r);
                    color.setBlue(g);
                }
                writeCacheMachineExtruder(data.uniqueName(), color, i ? false: true);
                if (print_materials.size() > i)
                {
                    writeCacheExtruderMaterialIndex(data.uniqueName(), i, print_materials[i]);
                }
            }
        }
        writeCacheCurrentProfile(machine_unique_name, default_print_profile);

        m_machineDatas.append(data);
        //addMachineMeta(data);
        if (machine_exist)
        {
            _addMachineFromUniqueName(data.uniqueName());
        }
        else
        {
            PrintMachine* machine = new PrintMachine(nullptr, data);
            _addImportMachine(machine);
        }
        setCurrBedType(curr_bed_type);
        setPrintSequence(print_sequence);
        setFirstLayerPrintSequence(first_layer_print_sequence);
    }

    bool ParameterManager::exportAllCurrentSettings(const QString& file_path)
    {
        if (!m_currentMachine)
        {
            return false;
        }
        QString machine_file = QString("%1/%2.def.json").arg(file_path).arg(m_currentMachine->uniqueName());
        m_currentMachine->exportSetting(machine_file);

        auto material_path = (file_path + "/materials");
        QDir().mkpath(material_path);
        for (const auto& extruder : m_currentMachine->extruders())
        {
            auto material = m_currentMachine->materialWithName(extruder->materialName());
            if (!material)
            {
                continue;
            }
            auto material_file = QString("%1/%2.json").arg(material_path).arg(extruder->materialName());
            material->exportSetting(material_file);
        }

        auto profile_path = file_path + "/processes";
        QDir().mkpath(profile_path);
        auto profile = m_currentMachine->currentProfile();
        if (profile)
        {
            auto profile_file = QString("%1/%2.json").arg(profile_path).arg(profile->name());
            profile->exportSetting(profile_file);
        }
        return true;
    }

    void creative_kernel::ParameterManager::loadParamFromProject(const QString& zip_file_name)
    {
        QTemporaryDir temp_dir;
        auto extract_file_name = temp_dir.filePath("extract_param/");
        qtuser_core::extractDir(zip_file_name, extract_file_name);

        QString machine_name;
        QString machine_path;
        QDir dir(extract_file_name);
        for (const auto& entry : dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot)) {
            machine_name = entry.fileName();
            machine_name.truncate(machine_name.lastIndexOf(".def.json"));
            machine_path = entry.absoluteFilePath();
        }
        if (machine_name.isEmpty())
        {
            return;
        }
        bool machine_exist = false;
        QString machine_file_name;
        machine_file_name = defaultMachineFile(machine_name, true);
        makeSureUserParamPackDir(machine_name);
        for (const auto& machine_data : m_machineDatas)
        {
            if (machine_data.uniqueName() == machine_name)
            {
                machine_exist = true;
                break;
            }
        }
        if (!machine_exist)
        {
            MachineData data;
            int pos = machine_name.lastIndexOf("-");
            data.isUser = true;
            data.baseName = machine_name.mid(0, pos);
            data.codeName = data.baseName;
            data.extruderDiameters << machine_name.mid(pos+1).toFloat();
            m_machineDatas.append(data);

            auto dstPath = QString("%1/%2/").arg(_pfpt_user_parampack).arg(data.uniqueName());
            qtuser_core::extractDir(zip_file_name, dstPath);
            addMachineMeta(data);
            PrintMachine* machine = new PrintMachine(nullptr, data);
            QMetaObject::invokeMethod(this, [this, machine]() {
                _addImportMachine(machine);
            }, Qt::ConnectionType::QueuedConnection);
            return;
        }
        us::USettings* settings_org = nullptr;
        us::USettings* settings_prj = nullptr;
        us::DefaultLoader loader;
        QList<us::USettings*>* extruderSettingsPrj = new QList<us::USettings*>;
        us::USettings* machineSettingsPrj = new us::USettings;
        loader.loadDefault(machine_path, machineSettingsPrj, extruderSettingsPrj);

        QList<us::USettings*>* extruderSettingsOrg = new QList<us::USettings*>;
        us::USettings* machineSettingsOrg = new us::USettings;

        auto machine = _findMachine(machine_name);
        if (machine)
        {
            settings_org = machine->settings();
        }
        else
        {
            loader.loadDefault(defaultMachineFile(machine_name), machineSettingsOrg, extruderSettingsOrg);
            settings_org = machineSettingsOrg;
        }
        settings_prj = machineSettingsPrj;

        auto saveDiff = [](us::USettings* org, us::USettings* prj, QString filename) {
            const QHash<QString, us::USetting*>& settings = prj->settings();
            auto it = settings.begin();
            us::USettings* userSettings = new us::USettings;
            while (it != settings.end())
            {
                QString value1 = it.value()->str();
                QString value2 = org->value(it.key(), "");
                if (it.value()->str() != value2)
                {
                    userSettings->add(it.key(), value1, true);
                }
                it++;
            }
            userSettings->saveAsDefault(filename);
            delete userSettings;
        };
        saveDiff(settings_org, settings_prj, machineCoverFile(machine_name));

        QString material_name;
        QString material_path;
        QDir material_dir(QString("%1/materials").arg(extract_file_name));
        for (const auto& entry : material_dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot)) {
            material_name = entry.completeBaseName();
            material_path = entry.absoluteFilePath();

            auto material_names = fetchFileNames((QString("%1/%2/Materials").arg(_pfpt_default_parampack).arg(machine_name)), true);
            if (!material_names.contains(material_name))
            {
                QFile(material_path).copy(defaultMaterialFile(machine_name, material_name, true));
            }
            else
            {
                us::USettings* materialSettingsPrj = new us::USettings;
                loader.loadDefault(material_path, materialSettingsPrj);
                us::USettings* materialSettingsOrg = new us::USettings;
                loader.loadDefault(defaultMaterialFile(machine_name, material_name), materialSettingsOrg);

                saveDiff(materialSettingsOrg, materialSettingsPrj, materialCoverFile(machine_name, material_name));
            }
        }

        removeCacheMachine(machine_name);
        QFile jsonFile(machine_path);
        if (!jsonFile.exists() || !jsonFile.open(QIODevice::ReadOnly))
        {
            return;
        }
        rapidjson::Document document;
        document.Parse(jsonFile.readAll().constData());
        if (document.HasParseError())
        {
            return;
        }
        if (document.HasMember("extruders") && document["extruders"].IsArray())
        {
            const auto& array = document["extruders"].GetArray();
            for (size_t i = 0; i < array.Size(); i++)
            {
                const auto& extruder_obj = array[i].GetObj();
                if (extruder_obj.HasMember("metadata") && extruder_obj["metadata"].IsObject())
                {
                    const auto& meta_obj = extruder_obj["metadata"].GetObj();
                    writeCacheMachineExtruder(machine_name, meta_obj["color"].GetInt64(), i ? false : true);
                    writeCacheExtruderMaterialIndex(machine_name, i, meta_obj["current_material"].GetString());
                }
            }
        }

        QString process_name;
        QString process_name_noext;
        QString process_path;
        QDir process_dir(QString("%1/processes").arg(extract_file_name));
        for (const auto& entry : process_dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot)) {
            process_name = entry.fileName();
            process_name_noext = entry.completeBaseName();
            process_path = entry.absoluteFilePath();
        }

        auto profile_names = fetchOfficialProfileNames(machine_name);
        if (!profile_names.contains(process_name))
        {
            QFile(process_path).copy(userProfileFile(machine_name, process_name));
        }
        else
        {
            us::USettings* processSettingsPrj = new us::USettings;
            loader.loadDefault(process_path, processSettingsPrj);
            us::USettings* processSettingsOrg = new us::USettings;
            loader.loadDefault(defaultProfileFile(machine_name, process_name), processSettingsOrg);

            saveDiff(processSettingsOrg, processSettingsPrj, userProfileCoverFile(machine_name, process_name));
        }

        writeCacheCurrentProfile(machine_name, process_name_noext);

        QMetaObject::invokeMethod(this, [this, machine_name = std::move(machine_name)]() {
            auto machine = _createMachine(machine_name);
            _addImportMachine(machine);
            }, Qt::ConnectionType::QueuedConnection);

    }

    QString ParameterManager::getCodeNameByBaseName(const QString& baseName)
    {
        for (auto machine : m_machines)
        {
            if (machine->codeName().contains(baseName))
            {
                return machine->codeName();
            }
        }

        return QString();
    }
    Q_INVOKABLE QString ParameterManager::getBaseNameByUniqueName(const QString& uniqueName)
    {
        for (auto machine : m_machines)
        {
            if (machine->uniqueName() == uniqueName)
            {
                return machine->baseName();
            }
        }
        return uniqueName;
    }

    Q_INVOKABLE QString ParameterManager::getNameByUniqueName(const QString& uniqueName)
    {
        for (auto machine : m_machines)
        {
            if (machine->uniqueName() == uniqueName)
            {
                return machine->name();
            }
        }
        return uniqueName;
    }

    void ParameterManager::setProfileAdvanced(bool checked)
    {
        QSettings settings;
        int setCheck = checked ? 1 : 0;
        settings.beginGroup(QStringLiteral("Advanced"));
        settings.setValue(QStringLiteral("profileAdvanced"), setCheck);
        settings.endGroup();
    }

    bool ParameterManager::getProfileAdvanced()
    {
        QSettings setting;
        setting.beginGroup("Advanced");
        int check = setting.value("profileAdvanced", 0).toInt();
        setting.endGroup();
        bool setCheck = check == 1 ? true : false;
        return setCheck;
    }

    void ParameterManager::setMachineAdvanced(bool checked)
    {
        QSettings settings;
        int setCheck = checked ? 1 : 0;
        settings.beginGroup(QStringLiteral("Advanced"));
        settings.setValue(QStringLiteral("machineAdvanced"), setCheck);
        settings.endGroup();
    }

    bool ParameterManager::getMachineAdvanced()
    {
        QSettings setting;
        setting.beginGroup("Advanced");
        int check = setting.value("machineAdvanced", 0).toInt();
        setting.endGroup();
        bool setCheck = check == 1 ? true : false;
        return setCheck;
    }

    bool ParameterManager::settingsDirty()
    {
        if (m_currentMachine)
            return m_currentMachine->machineDirty();
        return false;
    }

    void ParameterManager::clearSettingsDirty()
    {
        if (m_currentMachine)
            m_currentMachine->clearMachineDirty();
    }

    QSharedPointer<us::USettings> ParameterManager::createCurrentGlobalSettings()
    {
        if (!m_currentMachine)
            return QSharedPointer<us::USettings>(createDefaultGlobal());

        return m_currentMachine->createCurrentGlobalSettings();
    }

    QList<QSharedPointer<us::USettings>> ParameterManager::createCurrentExtruderSettings()
    {
        if (!m_currentMachine)
            return QList<QSharedPointer<us::USettings>>();

        return m_currentMachine->createCurrentExtruderSettings();
    }

    void ParameterManager::_addMachine(PrintMachine *machine)
    {
        if (!machine)
            return;
        auto machine_name = machine->uniqueName();
        removeCacheMachine(machine_name);
        makeSureUserParamPackDir(machine_name);
        if (m_currentMachine)
        {
            for (int i = 0; i < m_currentMachine->extruderAllCount(); ++i)
            {
                auto extruder = m_currentMachine->extruders()[i];
                writeCacheMachineExtruder(machine->uniqueName(), extruder->color(), extruder->physical());
                writeCacheExtruderMaterialIndex(machine->uniqueName(), i, extruder->materialName());
            }
        }

        machine->setParent(this);
        machine->added();
        if(machine->isDefault())
        {
            m_machines.push_back(machine);
        }else{
            m_machines.push_front(machine);
        }
        m_MachineListModel->insertMachine(machine, m_machines.indexOf(machine));

        //_cacheMachine(machine->uniqueName());
        emit machineNameListChanged();
        emit userMachineNameListChanged();
        emit defaultMachineNameListChanged();
        emit machinesCountChanged();
        _setCurrentMachine(machine);
    }

    void ParameterManager::_addImportMachine(PrintMachine *machine)
    {
        if (!machine)
            return;

        machine->setParent(this);
        machine->added(true);
        if(machine->isDefault())
        {
            m_machines.push_back(machine);
        }else{
            m_machines.push_front(machine);
        }
        m_MachineListModel->insertMachine(machine, m_machines.indexOf(machine));

        //_cacheMachine(machine->uniqueName());
        emit machineNameListChanged();
        emit userMachineNameListChanged();
        emit defaultMachineNameListChanged();
        emit machinesCountChanged();
        _setCurrentMachine(machine);
        machine->filterMeterialsFromTypes();
    }

    void ParameterManager::_addMachineFromUniqueName(const QString &uniqueName)
    {
        sensorAnlyticsTrace("Add & Manage 3D Printer",
                            QString("Add Printer [%1]").arg(uniqueName));

        PrintMachine *machine = _findMachine(uniqueName);
        if (machine)
        {
            qWarning() << QString("ParameterManager::addMachine error [%1].")
                              .arg(uniqueName);
            machine->reload();
            _setCurrentMachine(machine);
            return;
        }

        machine = _createMachine(uniqueName);
        _addMachine(machine);
        machine->filterMeterialsFromTypes();
        Q_EMIT machineListChange();
    }

    void ParameterManager::_setCurrentMachine(PrintMachine *machine, bool needEmit)
    {
        if (m_currentMachine == machine)
        {
            qDebug() << QString("ParameterManager::_setCurrentMachine same machine [%1]")
                            .arg(machine ? machine->uniqueName() : QString(""));
            return;
        }

        bool change_size = false;
        if (m_currentMachine)
        {
            change_size = m_currentMachine->isBelt() ^ machine->isBelt();
            disconnect(m_currentMachine, &PrintMachine::extruderColorChanged, this, &ParameterManager::currentColorsChanged);
            disconnect(m_currentMachine, &PrintMachine::extruderClearanceRadiusChanged, this, &ParameterManager::extruderClearanceRadiusChanged);
            disconnect(m_currentMachine, &PrintMachine::extruderClearanceHeightToLidChanged, this, &ParameterManager::extruderClearanceHeightToLidChanged);
            disconnect(m_currentMachine, &PrintMachine::extruderClearanceHeightToRodChanged, this, &ParameterManager::extruderClearanceHeightToRodChanged);
            disconnect(m_currentMachine, &PrintMachine::currentBedTypeChanged, this, &ParameterManager::currBedTypeChanged);
        }

        m_currentMachine = machine;
        emit curMachineIndexChanged();
        s_currentMachine = machine;
        QQmlEngine::setObjectOwnership(m_currentMachine, QQmlEngine::QQmlEngine::CppOwnership);
        connect(m_currentMachine, &PrintMachine::extruderColorChanged, this, &ParameterManager::currentColorsChanged);
        connect(m_currentMachine, &PrintMachine::extruderClearanceRadiusChanged, this, &ParameterManager::extruderClearanceRadiusChanged);
        connect(m_currentMachine, &PrintMachine::extruderClearanceHeightToLidChanged, this, &ParameterManager::extruderClearanceHeightToLidChanged);
        connect(m_currentMachine, &PrintMachine::extruderClearanceHeightToRodChanged, this, &ParameterManager::extruderClearanceHeightToRodChanged);
        connect(m_currentMachine, &PrintMachine::currentBedTypeChanged, this, &ParameterManager::currBedTypeChanged);

        applyPrintMachine(this->m_currentMachine);
        if (m_currentMachine) {
            m_currentMachine->setDirty();
        }

        PrintProfile* profile = qobject_cast<PrintProfile*>(m_currentMachine->currentProfileObject());
        if (profile)
        {
            disconnect(profile, &PrintProfile::enablePrimeTowerChanged, this, &ParameterManager::enablePrimeTowerChanged);
            disconnect(profile, &PrintProfile::currentProcessPrimeVolumeChanged, this, &ParameterManager::currentProcessPrimeVolumeChanged);
            disconnect(profile, &PrintProfile::currentProcessPrimeTowerWidthChanged, this, &ParameterManager::currentProcessPrimeTowerWidthChanged);
            disconnect(profile, &PrintProfile::currentProcessLayerHeightChanged, this, &ParameterManager::currentProcessLayerHeightChanged);
            disconnect(profile, &PrintProfile::enableSupportChanged, this, &ParameterManager::enableSupportChanged);
            disconnect(profile, &PrintProfile::currentProcessSupportFilamentChanged, this, &ParameterManager::currentProcessSupportFilamentChanged);
            disconnect(profile, &PrintProfile::currentProcessSupportInterfaceFilamentChanged, this, &ParameterManager::currentProcessSupportInterfaceFilamentChanged);
            disconnect(profile, &PrintProfile::printSequenceChanged, this, &ParameterManager::printSequenceChanged);
        }
        m_currentMachine->_getCurrentProfile();
        profile = qobject_cast<PrintProfile*>(m_currentMachine->currentProfileObject());
        if (profile)
        {
            connect(profile, &PrintProfile::enablePrimeTowerChanged, this, &ParameterManager::enablePrimeTowerChanged);
            connect(profile, &PrintProfile::currentProcessPrimeVolumeChanged, this, &ParameterManager::currentProcessPrimeVolumeChanged);
            connect(profile, &PrintProfile::currentProcessPrimeTowerWidthChanged, this, &ParameterManager::currentProcessPrimeTowerWidthChanged);
            connect(profile, &PrintProfile::currentProcessLayerHeightChanged, this, &ParameterManager::currentProcessLayerHeightChanged);
            connect(profile, &PrintProfile::enableSupportChanged, this, &ParameterManager::enableSupportChanged);
            connect(profile, &PrintProfile::currentProcessSupportFilamentChanged, this, &ParameterManager::currentProcessSupportFilamentChanged);
            connect(profile, &PrintProfile::currentProcessSupportInterfaceFilamentChanged, this, &ParameterManager::currentProcessSupportInterfaceFilamentChanged);
            connect(profile, &PrintProfile::printSequenceChanged, this, &ParameterManager::printSequenceChanged);
        }
        setFunctionType(0);

        _cacheCurrentMachineIndex();

        if (change_size)
        {
            creative_kernel::Nest2DJobEx* job = new Nest2DJobEx();
            // creative_kernel::Nest2DJob *job = new Nest2DJob(this);
            QList<qtuser_core::JobPtr> jobs;
            //if (currentMachineIsBelt())
            //{
            //    job->setNestType(cxkernel::NestPlaceType::ONELINE);
            //}
            jobs.push_back(qtuser_core::JobPtr(job));
            cxkernel::executeJobs(jobs);
        }

        syncCurParamToVisScene();
        if (needEmit)
            emit curMachineIndexChanged();
    }

    void ParameterManager::_removeMachine(PrintMachine *machine)
    {
        if (!machine)
            return;

        if (!m_machines.contains(machine))
        {
            qWarning() << QString("ParameterManager::_removeMachine not added [%1]")
                              .arg(machine->uniqueName());
            return;
        }

        m_MachineListModel->removeMachine(machine, m_machines.indexOf(machine));
        machine->removed();
        m_machines.removeOne(machine);

        _removeMachine(machine->uniqueName());
        emit machineNameListChanged();
        emit userMachineNameListChanged();
        emit defaultMachineNameListChanged();
        emit machinesCountChanged();
        Q_EMIT machineListChange();


        PrintMachine *nextMachine = nullptr;

        if (m_currentMachine == machine)
        {

            if (m_machines.count() > 0)
            {
                nextMachine = m_machines.at(0);
                machine->deleteLater();
            }
            else
            {
                //qml自动回收
                QQmlEngine::setObjectOwnership(machine, QQmlEngine::JavaScriptOwnership);
            }
            m_currentMachine = nullptr;
        }

        if (nextMachine)
            _setCurrentMachine(nextMachine);
        emit curMachineIndexChanged();
    }

    bool ParameterManager::_checkMachineExist(const QString &uniqueName)
    {
        PrintMachine *machine = _findMachine(uniqueName);
        return machine != nullptr;
    }

    PrintMachine *ParameterManager::_createMachine(const QString& uniqueName)
    {
        MachineData data;
        for (const auto& machineData: m_machineDatas)
        {
            if (machineData.uniqueName() == uniqueName)
            {
                data = machineData;
                break;
            }
        }
        if (data.baseName.isEmpty())
        {
            return nullptr;
        }
        PrintMachine *machine = new PrintMachine(nullptr, data);
        return machine;
    }

    void ParameterManager::_removeMachine(const QString &machine)
    {
        int index = -1;
        for (int i = 0; i < m_machineDatas.size(); ++i)
        {
            auto data = m_machineDatas[i];
            if (!data.isUser)
            {
                continue;
            }
            if (data.uniqueName() == machine)
            {
                index = i;
                break;
            }
        }
        if (index != -1)
        {
            m_machineDatas.removeAt(index);
        }
        removeCacheMachine(machine);
    }

    void ParameterManager::_cacheCurrentMachineIndex()
    {
        int currentMachineIndex = m_machines.indexOf(m_currentMachine);
        writeCacheCurrentMachineIndex(currentMachineIndex);
    }

    QStringList ParameterManager::_machineNames(const QList<PrintMachine *> &machines)
    {
        QStringList nameList = cxkernel::objectNames<PrintMachine>(machines);
        return nameList;
    }

    QStringList ParameterManager::_machineUniqueNames(const QList<PrintMachine *> &machines)
    {
        return cxkernel::objectUniqueNames<PrintMachine>(machines);
    }

    PrintMachine *ParameterManager::_findMachine(const QString &uniqueName)
    {
        return cxkernel::findObject<PrintMachine>(uniqueName, m_machines);
    }

    QString ParameterManager::_uniqueNameFromBaseNameNozzle(const QString &baseName, const QString &nozzle)
    {
        if (baseName.isEmpty())
            return QString();
        return QString("%1-%2").arg(baseName).arg(nozzle);
    }

    QString ParameterManager::machineNozzleSelectList(const QString machineName)
    {
        if (!_machineNames(m_machines).empty())
        {
            QString result;
            for (const QString &machine : _machineNames(m_machines))
            {

                QString machineStr = machine;
                QString nozzleStr = " nozzle";
                machineStr.remove(nozzleStr);
                int lastDashPos = machineStr.lastIndexOf("-");
                if (machineStr.left(lastDashPos) == machineName)
                {
                    result += machineStr.right(lastDashPos+1) + "-";
                }
            }
            return result;
        }
        return QString();
    }

    QStringList ParameterManager::machineNameList()
    {
        return _machineNames(m_machines);
        /* QStringList machines;
         for (PrintMachine* machine : m_machines)
         {
             if (!machines.contains(machine->name()))
             {
                 machines << machine->name();
             }
         }
         return machines;*/
    }

    QStringList ParameterManager::userMachineNameList()
    {
        QStringList machineList = machineNameList();
        QStringList userMachines;
        for (QString machineName : machineList)
        {
            if (isMachineInUser(machineName))
            {
                userMachines << machineName;
            }
        }

        return userMachines;
    }

    QStringList ParameterManager::defaultMachineNameList()
    {
        QStringList machineList = machineNameList();
        QStringList defaultMachines;
        for (QString machineName : machineList)
        {
            if (isMachineInFactory(machineName))
            {
                defaultMachines << machineName;
            }
        }

        return defaultMachines;
    }

    QObject* ParameterManager::machineListModelObject()
    {
        return machineListModel();
    }

    PrintMachineListModel* ParameterManager::machineListModel()
    {
        return m_MachineListModel;
    }

    QStringList ParameterManager::currentProfiles()
    {
        if (m_currentMachine)
            return m_currentMachine->profileNames();
        else
            return QStringList();
    }
    void ParameterManager::setCurrentProfile(QString profile)
    {
        profile = profile.replace(".default", "");
        int index = m_currentMachine->profileNames().indexOf(profile);
        if (index >= 0)
        {
            m_currentMachine->setCurrentProfile(index);
        }
        else
        {
            m_currentMachine->setCurrentProfile(0);
        }
    }
    void ParameterManager::setExtrudersMaterial(int iExtruder, const QString &material)
    {
        m_currentMachine->setExtruderMaterial(iExtruder, material);
    }
    QString ParameterManager::currentProfile()
    {
        if (m_currentMachine)
        {
            int index = m_currentMachine->curProfileIndex();
            if (index >= 0 && index < currentProfiles().size())
                return currentProfiles().at(index);
        }

        return "";
    }
    QStringList ParameterManager::currentExtruders()
    {
        QStringList extruders;
        if (m_currentMachine)
        {
            for (int i = 0; i < m_currentMachine->extruderCount(); i++)
            {
                QString extruder = userExtruderFile(m_currentMachine->uniqueName(), i);
                extruders.append(extruder);
                // PrintExtruder* extruder = qobject_cast<PrintExtruder*>(m_currentMachine->extruderObject(i));
                // extruder->materialName();
            }
        }
        return extruders;
    }

    std::vector<trimesh::vec> ParameterManager::currentColors()
    {
        std::vector<trimesh::vec> colors;
        if (m_currentMachine == NULL)
        {
            // colors.push_back(trimesh::vec(0.09, 0.80, 0.37));
            colors.push_back(trimesh::vec(0.00, 0.00, 0.00));
            return colors;
        }

        int counr = m_currentMachine->extruderAllCount();
        for (int i = 0; i < counr; i++)
        {
            QColor color = m_currentMachine->extruderColor(i);
            colors.push_back(trimesh::vec(color.red() / 255.0f, color.green() / 255.0f, color.blue() / 255.0f));
        }
        return colors;
    }

    QStringList ParameterManager::currrentBedKeys()
    {
        auto settings = createDefaultGlobal();
        return settings->enumKeys("curr_bed_type");
    }

    QVariantList ParameterManager::currrentBedTypes()
    {
        auto settings = createDefaultGlobal();
        return settings->enums("curr_bed_type");
    }

    QStringList ParameterManager::currentMaterials()
    {
        QStringList materials;
        if (m_currentMachine)
        {
            for (int i = 0; i < m_currentMachine->extruderCount(); i++)
            {
                PrintExtruder *extruder = qobject_cast<PrintExtruder *>(m_currentMachine->extruderObject(i));
                materials.append(extruder->materialName());
            }
        }
        return materials;
    }

    QString ParameterManager::currentMaterialsType(int index)
    {
        PrintExtruder* extruder = qobject_cast<PrintExtruder*>(m_currentMachine->extruderObject(index));
        return extruder->curMaterial()->materialType();
    }

    void ParameterManager::onCurrentMachineChanged() {
        parameterEdited(m_currentMachine, {});

        emit extruderClearanceRadiusChanged(extruder_clearance_radius());
        emit extruderClearanceHeightToLidChanged(extruderClearanceHeightToLid());
        emit extruderClearanceHeightToRodChanged(extruderClearanceHeightToRod());
        emit enablePrimeTowerChanged(enablePrimeTower());
        emit currentProcessLayerHeightChanged(currentProcessLayerHeight());
        emit currentProcessPrimeVolumeChanged(currentProcessPrimeVolume());
        emit currentProcessPrimeTowerWidthChanged(currentProcessPrimeTowerWidth());
        emit enableSupportChanged(enableSupport());
        emit currentProcessSupportFilamentChanged(currentProcessSupportFilament());
        emit currentProcessSupportInterfaceFilamentChanged(currentProcessSupportInterfaceFilament());
        emit printSequenceChanged(printSequence());
        emit currBedTypeChanged(currBedType());
    }

    void ParameterManager::onParameterEdited() {
      for (const auto& connection : context_connections_) {
        disconnect(connection);
      }

      context_connections_.clear();

      if (!m_currentMachine) {
        return;
      }

      std::set<ParameterBase*> parameter_bases{};

      auto* machine = m_currentMachine;
      parameter_bases.emplace(machine);
      parameter_bases.emplace(machine->currentProfile());
      context_connections_.push_back(connect(
        machine, &PrintMachine::curProfileIndexChanged, this, [this, machine]() {
          parameterEdited(machine->currentProfile(), {});
        }));

      for (auto* extruder : machine->extruders()) {
        parameter_bases.emplace(extruder);
        parameter_bases.emplace(machine->materialAt(extruder->materialIndex()));
        context_connections_.push_back(connect(
          extruder, &PrintExtruder::materialIndexChanged, this, [this, machine, extruder]() {
            parameterEdited(machine->materialAt(extruder->materialIndex()), {});
          }));
      }

      for (auto* parameter_base : parameter_bases) {
        if (!parameter_base) {
          continue;
        }

        auto* settings = parameter_base->userSettings();
        const auto slot = [this, parameter_base](const QString& key) {
          parameterEdited(parameter_base, key);
        };

        context_connections_.push_back(connect(settings, &us::USettings::settingsChanged, this, slot));
        context_connections_.push_back(connect(settings, &us::USettings::settingValueChanged, this, slot));
      }

    }

    QString ParameterManager::getModifyModelDataFromType(const QString type)
    {
        QJsonArray obj_root;

        //get modified machine
        PrintMachine *modifiedMachine = nullptr;

        auto FindUiFile =[](const QString& file_name) {
            QByteArray json;
            const auto dir_path = getKernel()->globalConst()->getUiParameterDirPath();
             QString jsonfilename = QStringLiteral("%1/%2").arg(dir_path).arg(file_name);
             QFile file(jsonfilename);
             if (file.open(QIODevice::ReadOnly | QIODevice::Text))
             {
                 json = file.readAll();
                 file.close();

             }
             return  QJsonDocument::fromJson(json);


        };
        auto getKeyInfo = [](const QList<QString>& keylist,QJsonDocument& uidoc,QJsonArray& root, us::USettings* usettings, us::USettings* settings) {
            if(uidoc.isObject())
            {
                QJsonObject jObject = uidoc.object();
                QStringList keys = jObject.keys();
                if(jObject.contains("children"))
                {
                    QJsonArray categorys = jObject.value("children").toArray();
                    for(int i=0;i<categorys.size();i++){
                        QJsonObject category = categorys[i].toObject();
                        if (category.value("label").toString() == "Extruder_2")
                        {
                            continue;
                        }
                        qDebug() << category.value("label").toString();
                        QJsonObject outCategory;
                        if(category.contains("children"))
                        {
                            QJsonArray groups = category.value("children").toArray();
                            QJsonArray outgroups;
                            for(int j=0;j<groups.size();j++){
                                QJsonObject group = groups[j].toObject();

                                QJsonObject outgroup;
                                if(group.contains("children"))
                                {
                                    QJsonArray children = group.value("children").toArray();
                                    QJsonArray outChildren;
                                    for(int k=0;k<children.size();k++){
                                        if(children[k].isObject())
                                        {
                                            QJsonObject node = children[k].toObject();
                                            if(node.contains("key"))
                                            {
                                                QString childkey = node.value("key").toString() ;
                                                QString component = "";
                                                if(node.contains("component"))
                                                {
                                                    component = node.value("component").toString() ;
                                                }
                                                int index = keylist.indexOf(childkey);
                                                if(index>=0)
                                                {
                                                    QJsonObject item;
                                                    item["key"] = childkey;

                                                    us::USetting *usetting = usettings->findSetting(childkey);
                                                    us::USetting* setting = settings->findSetting(childkey);
                                                    if (usetting)
                                                    {
                                                        item["label"] = QCoreApplication::translate("ParameterComponent", usetting->def()->label.toLatin1());
                                                        if (usetting->def()->type == "coEnum" || usetting->def()->type == "coEnums")
                                                        {
                                                            for (auto option : usetting->def()->options)
                                                            {
                                                                if (option.first == usetting->str())
                                                                {
                                                                    item["NewValue"] = QCoreApplication::translate("ParameterComponent", option.second.toLatin1());

                                                                }
                                                                if (option.first == setting->str())
                                                                {
                                                                    item["OlderValue"] = QCoreApplication::translate("ParameterComponent", option.second.toLatin1());
                                                                }
                                                            }
                                                        }
                                                        else {
                                                            QString newValue = component == "check_box" ? (usettings->value(childkey, "") == "1" ? "true" : "false") : usettings->value(childkey, "");
                                                            item["NewValue"] = QCoreApplication::translate("ParameterComponent", newValue.toLatin1());
                                                            QString olderValue = component == "check_box" ? (settings->value(childkey, "") == "1" ? "true" : "false") : settings->value(childkey, "");
                                                            item["OlderValue"] = QCoreApplication::translate("ParameterComponent", olderValue.toLatin1());
                                                        }
                                                        outChildren.append(item);
                                                    }


                                                }
                                            }
                                            else {
                                                if (node.contains("children"))
                                                {

                                                    QString parentLabel = node.value("label").toString();;

                                                    parentLabel = QCoreApplication::translate("ParameterComponent", parentLabel.toLatin1())+":";


                                                    QJsonArray childkeys = node.value("children").toArray();
                                                    for (int m = 0; m < childkeys.size(); m++) {
                                                        if (childkeys[m].toObject().contains("key"))
                                                        {
                                                            QString childkey = childkeys[m].toObject().value("key").toString();
                                                            int index = keylist.indexOf(childkey);
                                                            QString component = "";
                                                            if (childkeys[m].toObject().contains("component"))
                                                            {
                                                                component = childkeys[m].toObject().value("component").toString();
                                                            }
                                                            if (index >= 0)
                                                            {
                                                                QJsonObject item;
                                                                item["key"] = childkey;

                                                                us::USetting* usetting = usettings->findSetting(childkey);
                                                                us::USetting* setting = settings->findSetting(childkey);
                                                                if (usetting)
                                                                {
                                                                    item["label"] = parentLabel + QCoreApplication::translate("ParameterComponent", usetting->def()->label.toLatin1());
                                                                    if (usetting->def()->type == "coEnum")
                                                                    {
                                                                        for (auto option : usetting->def()->options)
                                                                        {
                                                                            if (option.first == usetting->str())
                                                                            {
                                                                                item["NewValue"] = QCoreApplication::translate("ParameterComponent", option.second.toLatin1());

                                                                            }
                                                                            if (option.first == setting->str())
                                                                            {
                                                                                item["OlderValue"] = QCoreApplication::translate("ParameterComponent", option.second.toLatin1());
                                                                            }
                                                                        }
                                                                    }
                                                                    else {
                                                                        QString newValue = component == "check_box" ? (usettings->value(childkey, "") == "1" ? "true" : "false") : usettings->value(childkey, "");
                                                                        item["NewValue"] = QCoreApplication::translate("ParameterComponent", newValue.toLatin1());
                                                                        QString olderValue = component == "check_box" ? (settings->value(childkey, "") == "1" ? "true" : "false") : settings->value(childkey, "");
                                                                        item["OlderValue"] = QCoreApplication::translate("ParameterComponent", olderValue.toLatin1());
                                                                    }
                                                                    outChildren.append(item);
                                                                }
                                                            }
                                                        }
                                                    }
                                                }

                                            }
                                        }

                                    }
                                    if(outChildren.size()>0)
                                    {
                                        outgroup["label"] = QCoreApplication::translate("ParameterComponent", group["label"].toString().toLatin1());
                                        outgroup["children"] = outChildren;
                                    }
                                }
                                if(outgroup.contains("children"))
                                {
                                    outgroups.append(outgroup);
                                }
                            }
                            if(outgroups.size()>0)
                            {
                                outCategory["label"] = QCoreApplication::translate("ParameterComponent", category["label"].toString().toLatin1());;
                                outCategory["groups"] = outgroups;
                            }
                        }
                        if(outCategory.contains("groups"))
                        {
                            root.append(outCategory);
                        }



                    }
                }
            }
            int size = root.size();

        };
        for (auto machine : m_machines)
        {
            if (machine->isModified())
            {
                modifiedMachine = machine;
            }
        }
        if(modifiedMachine && (type=="all"|| type=="machine"))
        {
            QJsonDocument machineUiDoc = FindUiFile("machine.json");
            if(machineUiDoc.isEmpty() || machineUiDoc.isNull())
            {
                return "";
            }
            us::USettings* machineUserSettings = modifiedMachine->userSettings();
            QList<QString> keylist = machineUserSettings->hashSettings().keys();
            getKeyInfo(keylist,machineUiDoc,obj_root,machineUserSettings,modifiedMachine->settings());
            const QList<PrintExtruder*> extruders = modifiedMachine->extruders();
            for(auto extruder: extruders)
            {
                //QJsonDocument extruderUiDoc = FindUiFile("machine.json");
                if(extruder->physical())
                {
                    us::USettings* extruderUserSettings = extruder->userSettings();
                    QList<QString> keylist = extruderUserSettings->hashSettings().keys();
                    getKeyInfo(keylist,machineUiDoc,obj_root,extruderUserSettings,extruder->settings());
                }

            }
        }
        if(currentMachine())
        {
            PrintMaterial *material = currentMachine()->materialWithName(currentMachine()->modifiedMaterialName());
            if(material && (type == "all" || type == "material"))
            {
                QJsonDocument materialUiDoc = FindUiFile("material.json");
                if(materialUiDoc.isEmpty() || materialUiDoc.isNull())
                {
                    return "";
                }
                us::USettings* userSettings = material->userSettings();
                QList<QString> keylist = userSettings->hashSettings().keys();
                getKeyInfo(keylist,materialUiDoc,obj_root,userSettings,material->settings());
            }
            PrintProfile *profile = currentMachine()->modifiedProcessObject();//currentMachine()->materialWithName(currentMachine()->modifiedMaterialName());
            if(profile && (type == "all" || type == "process"))
            {
                QJsonDocument processUiDoc = FindUiFile("process.json");
                if(processUiDoc.isEmpty() || processUiDoc.isNull())
                {
                    return "";
                }
                us::USettings* userSettings = profile->userSettings();
                QList<QString> keylist = userSettings->hashSettings().keys();
                getKeyInfo(keylist,processUiDoc,obj_root,userSettings,profile->settings());
            }
        }




        QJsonDocument jsonDocu(obj_root);
        QByteArray jsonData = jsonDocu.toJson();
        return QString::fromUtf8(jsonData);
    }

}
