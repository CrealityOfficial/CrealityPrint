#include "parametermanager.h"

#include <set>

#include <QPointer>
#include <QtCore/QUrl>
#include <QtCore/QFileInfo>

#include "cxkernel/interface/iointerface.h"
#include "interface/uiinterface.h"

#include "qtusercore/module/systemutil.h"

#include "us/settingdef.h"
#include <QStandardPaths>
#include "kernel/translator.h"

#include "interface/commandinterface.h"
#include "cxkernel/interface/iointerface.h"
#include "interface/appsettinginterface.h"
#include "job/nest2djob.h"
#include "cxkernel/interface/jobsinterface.h"

#include "interface/visualsceneinterface.h"
#include "internal/parameter/printmachine.h"
#include "internal/parameter/printmaterial.h"
#include "internal/parameter/parameterpath.h"
#include "internal/parameter/parametercache.h"
#include "internal/parameter/parameterutil.h"
#include "internal/parameter/printprofile.h"
#include  "internal/parameter/printextruder.h"
#include "qcxutil/util/traitsutil.h"

#include "qtusercore/string/resourcesfinder.h"
#include <QtCore>

namespace creative_kernel
{
    ParameterManager::ParameterManager(QObject* parent)
        : QObject(parent)
        , m_currentMachine(nullptr)
        , m_functionType(0)
    {
        m_emptyMachine = nullptr;
    }

    ParameterManager::~ParameterManager()
    {
    }

    void ParameterManager::intialize()
    {
        loadMachineMeta(m_machineMetas);
        loadMachineMeta(m_machineMetas, true);
        //us::SettingDef::instance().writeAllKeys();

        QStringList machineUniqueNames = readCacheMachineUniqueNames();

        qDebug() << QString("ParameterManager::intialize : ");
        qDebug() << machineUniqueNames;
        for (const QString& machineUniqueName : machineUniqueNames)
        {
            bool isUser = isUserMachine(machineUniqueName);
            PrintMachine* machine = _createMachine(machineUniqueName, QString(), isUser);
            if (machine)
            {
                machine->setParent(this);
                machine->added(true);
                m_machines.append(machine);
            }
        }
        emit machineNameListChanged();

        QSettings setting;
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

            setCurrentMachine(currentMachineIndex);
        }

        _cacheCurrentMachineIndex();
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

    void ParameterManager::addMachine(const QString& modelMachineName, const QString& nozzle)
    {
        _addMachineFromUniqueName(_uniqueNameFromBaseNameNozzle(modelMachineName, nozzle));
    }

    Q_INVOKABLE void ParameterManager::addUserMachine(const QString& modelMachineName, const QString& nozzle, const QString& newMachineName)
    {
        //1.读取模板信息到meta
        MachineMeta meta;
        MachineData data;
        if (!_checkUniqueName(_uniqueNameFromBaseNameNozzle(modelMachineName, nozzle), meta, data))
        {
            qWarning() << QString("ParameterManager::_checkUniqueName error. [%1]").arg(modelMachineName);
        }

        for (const MachineMeta& machineMeta : m_machineMetas)
        {
            if (machineMeta.baseName == modelMachineName)
            {
                meta = machineMeta;
                meta.isUser = true;
                meta.baseName = newMachineName;
                meta.basicMachineName = modelMachineName;
                break;
            }
        }
        //判断源机型是否为用户自定义
        bool isSourceUser = false;
        for (const MachineMeta& machineMeta : m_machineMetas)
        {
            if (machineMeta.baseName == modelMachineName)
            {
                isSourceUser = machineMeta.isUser;
            }
        }
        //2.添加机型信息到machines.json
        bool res = addMachineMeta(meta);
        QString oldMachineName = modelMachineName;
        if (isSourceUser)
        {
            oldMachineName = _uniqueNameFromBaseNameNozzle(modelMachineName, nozzle);
        }
        makeSureUserMachineFile(oldMachineName, _uniqueNameFromBaseNameNozzle(newMachineName, nozzle), isSourceUser);

        //3.调用原来的添加设备的流程
        PrintMachine* machine = new PrintMachine(nullptr, meta, data);
        _addMachine(machine);
    }

    Q_INVOKABLE void ParameterManager::importUserMachine(const QString& srcFilePath, const QString& userMachineName, const QString& machineBaseName, const QString& nozzle)
    {
        QString machineName;
        int idx = machineBaseName.indexOf("Creality");
        if (idx == 0)
        {
            machineName = machineBaseName.mid(9);
        }
        else {
            machineName = machineBaseName;
        }
        //1.读取模板信息到meta
        MachineMeta meta;
        MachineData data;
        if (!_checkUniqueName(_uniqueNameFromBaseNameNozzle(machineName, nozzle), meta, data))
        {
            qWarning() << QString("ParameterManager::_checkUniqueName error. [%1]").arg(machineName);
        }

        for (const MachineMeta& machineMeta : m_machineMetas)
        {
            if (machineMeta.baseName == machineName)
            {
                meta = machineMeta;
                meta.baseName = userMachineName;
                break;
            }
        }

        meta.isUser = true;
        meta.basicMachineName = machineBaseName;
        PrintMachine* machine = new PrintMachine(nullptr, meta, data, PrintMachine::PT_FROMDEFAULT);

        //2.添加机型信息到machines.json
        bool res = addMachineMeta(meta);

        //3.拷贝配置文件
        //makeSureUserMachineFile(machineName, _uniqueNameFromBaseNameNozzle(userMachineName, nozzle));

        QString userFile = userMachineFile(_uniqueNameFromBaseNameNozzle(userMachineName, nozzle));
        //写一个把传入的文件分拆成打印机和喷嘴两个文件的接口
        fileToMachineAndExtruder(QUrl(srcFilePath).toLocalFile(), userFile, userExtruderFile(_uniqueNameFromBaseNameNozzle(userMachineName, nozzle), 0));
        res = qtuser_core::copyFile(QUrl(srcFilePath).toLocalFile(), userFile, false);
        if (res)
        {
            //重命名文件
        }

        //4.调用原来的添加设备的流程
        _addImportMachine(machine);
        //_addMachineFromUniqueName(_uniqueNameFromBaseNameNozzle(userMachineName, nozzle));
    }

    Q_INVOKABLE bool ParameterManager::addMachineMeta(const MachineMeta& machineMeta)
    {
        m_machineMetas.append(machineMeta);
        saveMachineMetaUser(machineMeta);
        return true;
    }

    bool ParameterManager::isUserMachine(const QString& machineName)
    {
        bool res = false;
        for (MachineMeta data : m_machineMetas)
        {
            if (data.baseName == machineName.split("_")[0])
            {
                res = data.isUser;
            }
        }

        return res;
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

    bool ParameterManager::checkMachineExist(const QString& baseName, const QString& nozzle)
    {
        return _checkMachineExist(_uniqueNameFromBaseNameNozzle(baseName, nozzle));
    }

    void ParameterManager::setCurrentMachine(int index)
    {
        if (index < 0 || index >= m_machines.count())
        {
            qWarning() << QString("ParameterManager::removeMachine error index [%1].").arg(index);
            return;
        }

        _setCurrentMachine(m_machines.at(index));

        syncCurParamToVisScene();
    }

    void ParameterManager::removeMachine(QObject* machineObject)
    {
        PrintMachine* machine = qobject_cast<PrintMachine*>(machineObject);
        _removeMachine(machine);
    }

    QObject* ParameterManager::machineObject(int index)
    {
        if (index >= 0 && index < m_machines.count())
            return m_machines.at(index);
        return m_emptyMachine;
    }

    QObject* ParameterManager::currentMachineObject()
    {
        return m_currentMachine ? m_currentMachine : m_emptyMachine;
    }

    QString ParameterManager::currentMachineName()
    {
        return m_currentMachine ? m_currentMachine->uniqueName() : QString("INVALID MACHINE");
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
        return m_currentMachine? m_currentMachine->machineWidth() : 0.0f;
    }
    float ParameterManager::currentMachinedepth()
    {
        return m_currentMachine? m_currentMachine->machineDepth() : 0.0f;
    }
    float ParameterManager::currentMachineheight()
    {
        return m_currentMachine? m_currentMachine->machineHeight() : 0.0f;
    }

    void ParameterManager::fileToMachineAndExtruder(const QString& sourceFilePath, const QString& machinePath, const QString& extruderPath)
    {
        QFile file(sourceFilePath);
        QString printerStr;
        QString extruderStr;
        QString& str = printerStr;

        int scope = 0;//0: machine 1. extruder
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
                else if(line == "[Extruder_0]" || line == "[Extruder_1]")
                {
                    scope = 1;
                    continue;
                }
                if (scope == 0)
                {
                    printerStr += line + "\n";
                }
                else {
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
        return m_currentMachine ? /*m_currentMachine->isBelt() || */m_currentMachine->centerIsZero() : false;
    }

    int ParameterManager::curMachineType()
    {
        return m_currentMachine ? m_currentMachine->machineType() : 1;
    }

    int ParameterManager::curExtruderCount()
    {
        if (!m_currentMachine)
            return 1;

        return m_currentMachine->extruderCount();
    }

    bool ParameterManager::currentMachineSupportExportImage()
    {
        if (!m_currentMachine)
            return false;

        return m_currentMachine->currentMachineSupportExportImage();
    }

    QObject* ParameterManager::findMachine(const QString& machineName)
    {
        QObject* temp = _findMachine(machineName);
        return temp;
    }

    QStringList ParameterManager::extruderSupportDiameters(const QString& machineBaseName, int index)
    {
        QStringList diameters;
        QString basename;
        for (const MachineMeta& meta : m_machineMetas)
        {
            int idx = machineBaseName.indexOf("Creality");
            if (idx == 0)
            {
                basename = machineBaseName.mid(9);
            }
            else {
                basename = machineBaseName;
            }
            int res = QString::compare(meta.baseName, basename, Qt::CaseInsensitive);
            if (res == 0)
            {
                if (index >= 0 && index < meta.extruderCount)
                {
                    for (float diameter : meta.supportExtruderDiameters[index])
                    {
                        diameters.append(QString("%1").arg(diameter));
                    }

                    break;
                }
            }
        }
        return diameters;
}

    Q_INVOKABLE QString creative_kernel::ParameterManager::getFileNameByPath(const QString& path)
    {
        //1.判断文件是否存在
        QUrl url(path);
        QFileInfo info(url.toLocalFile());
        QString baseName = info.baseName();
        return baseName;
    }

    Q_INVOKABLE QString creative_kernel::ParameterManager::getMachineName(const QString& path)
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
        //2.获取文件内部保存的机器名称
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

    int ParameterManager::getMachineExtruderCount(const QString& path)
    {
        //1.判断文件是否存在
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
        //2.获取文件内部保存的机器名称
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

    void ParameterManager::importPrinterFromQml(const QString& urlString)
    {
        QUrl url(urlString);

        QFileInfo info(url.toLocalFile());
        QString baseName = info.baseName();

        QString profileName = baseName;

        //
        PrintMachine* machine = _createMachine(urlString);
        if (machine)
        {
            machine->setParent(this);
            machine->added();
            m_machines.append(machine);
        }

        emit machineNameListChanged();
        emit machinesCountChanged();
    }

    bool ParameterManager::checkVisSceneSensitiveParam(const QString& key) const
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
        PrintProfile* profile{ nullptr };
        if (m_currentMachine) {
        profile = m_currentMachine->profile(m_currentMachine->curProfileIndex());
        }

        static const auto s2b = [](const QString& string) -> bool {
            return string == QStringLiteral("true");
        };

        const auto read_setting = [profile](const QString& key) -> QString {
            return profile ? profile->userSettings()->value(key,
                profile->settings()->value(key, {})) : QString{};
        };

        // 装填塔相关
        if (profile && s2b(read_setting(QStringLiteral("prime_tower_enable")))) {
            const auto brim_enable = s2b(read_setting(QStringLiteral("prime_tower_brim_enable")));
            const auto tower_size = read_setting(QStringLiteral("prime_tower_size")).toFloat() / 2;
            const auto brim_width = read_setting(QStringLiteral("brim_width")).toFloat();
            creative_kernel::showPrimeTower(
                read_setting(QStringLiteral("prime_tower_position_x")).toFloat(),
                read_setting(QStringLiteral("prime_tower_position_y")).toFloat(),
                brim_enable ? tower_size + brim_width : tower_size);
        }
        else {
            creative_kernel::hidePrimeTower();
        }
      // ...
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
            return  QSharedPointer<us::USettings>(createDefaultGlobal());

        return m_currentMachine->createCurrentGlobalSettings();
    }

    QList<QSharedPointer<us::USettings>> ParameterManager::createCurrentExtruderSettings()
    {
        if (!m_currentMachine)
            return QList<QSharedPointer<us::USettings>>();

        return m_currentMachine->createCurrentExtruderSettings();
    }

    void ParameterManager::_addMachine(PrintMachine* machine)
    {
        if (!machine)
            return;

        machine->setParent(this);
        machine->added();
        m_machines.append(machine);

        //_cacheMachine(machine->uniqueName());
        emit machineNameListChanged();
        emit machinesCountChanged();

        _setCurrentMachine(machine);
    }

    void ParameterManager::_addImportMachine(PrintMachine* machine)
    {
        if (!machine)
            return;

        machine->setParent(this);
        machine->addImport();
        m_machines.append(machine);

        //_cacheMachine(machine->uniqueName());
        emit machineNameListChanged();
        emit machinesCountChanged();

        _setCurrentMachine(machine);
    }

    void ParameterManager::_addMachineFromUniqueName(const QString& uniqueName)
    {
        sensorAnlyticsTrace("Add & Manage 3D Printer",
            QString("Add Printer [%1]").arg(uniqueName));

        PrintMachine* machine = _findMachine(uniqueName);
        if (machine)
        {
            qWarning() << QString("ParameterManager::addMachine error [%1].")
                .arg(uniqueName);
            _setCurrentMachine(machine);
            return;
        }

        machine = _createMachine(uniqueName);
        _addMachine(machine);
    }

    void ParameterManager::_setCurrentMachine(PrintMachine* machine)
    {
        if (m_currentMachine == machine)
        {
            qDebug() << QString("ParameterManager::_setCurrentMachine same machine [%1]")
                .arg(machine ? machine->uniqueName() : QString(""));
            return;
        }

        bool change_size = false;
        if(m_currentMachine)
            change_size = m_currentMachine->isBelt() ^ machine->isBelt();

        m_currentMachine = machine;
        applyPrintMachine(m_currentMachine);
        if (m_currentMachine)
            m_currentMachine->setDirty();

        emit curMachineIndexChanged();
        m_currentMachine->_getCurrentProfile();
        setFunctionType(0);

        _cacheCurrentMachineIndex();

        if (change_size)
        {
            creative_kernel::Nest2DJob* job = new Nest2DJob(this);
            QList<qtuser_core::JobPtr> jobs;
            if (currentMachineIsBelt())
            {
                job->setNestType(qcxutil::NestPlaceType::ONELINE);
            }
            jobs.push_back(qtuser_core::JobPtr(job));
            cxkernel::executeJobs(jobs);
        }
    }

    void ParameterManager::_removeMachine(PrintMachine* machine)
    {
        if (!machine)
            return;

        if (!m_machines.contains(machine))
        {
            qWarning() << QString("ParameterManager::_removeMachine not added [%1]")
                .arg(machine->uniqueName());
            return;
        }

        machine->removed();
        m_machines.removeOne(machine);

        _removeMachine(machine->uniqueName());
        emit machineNameListChanged();
        emit machinesCountChanged();

        machine->deleteLater();

        PrintMachine* nextMachine = nullptr;

        if (m_currentMachine == machine)
        {
            m_currentMachine = nullptr;
            if (m_machines.count() > 0)
                nextMachine = m_machines.at(0);
        }

        if (nextMachine)
            _setCurrentMachine(nextMachine);
    }

    bool ParameterManager::_checkMachineExist(const QString& uniqueName)
    {
        PrintMachine* machine = _findMachine(uniqueName);
        return machine != nullptr;
    }

    PrintMachine* ParameterManager::_createMachine(const QString& uniqueName, const QString& modelMachineName, bool isUser)
    {
        //uniqueName format         baseName_nozzle0-nozzle1 ...

        qDebug() << QString("ParameterManager::_createMachine [%1]").arg(uniqueName);
        if (_checkMachineExist(uniqueName))
        {
            qWarning() << QString("ParameterManager::_createMachine same name. [%1]").arg(uniqueName);
            return nullptr;
        }

        MachineMeta meta;
        MachineData data;
        meta.basicMachineName = modelMachineName == QString() ? uniqueName : modelMachineName;
        if (!_checkUniqueName(uniqueName, meta, data))
        {
            qWarning() << QString("ParameterManager::_checkUniqueName error. [%1]").arg(uniqueName);
            return nullptr;
        }

        meta.isUser = isUser;
        PrintMachine* machine = new PrintMachine(nullptr, meta, data);
        return machine;
    }

    void ParameterManager::_removeMachine(const QString& machine)
    {
        removeCacheMachine(machine);
    }

    void ParameterManager::_cacheCurrentMachineIndex()
    {
        int currentMachineIndex = m_machines.indexOf(m_currentMachine);
        writeCacheCurrentMachineIndex(currentMachineIndex);
    }

    QStringList ParameterManager::_machineNames(const QList<PrintMachine*>& machines)
    {
        return qcxutil::objectNames<PrintMachine>(machines);
    }

    QStringList ParameterManager::_machineUniqueNames(const QList<PrintMachine*>& machines)
    {
        return qcxutil::objectUniqueNames<PrintMachine>(machines);
    }

    PrintMachine* ParameterManager::_findMachine(const QString& uniqueName)
    {
        return qcxutil::findObject<PrintMachine>(uniqueName, m_machines);
    }

    bool ParameterManager::_checkUniqueName(const QString& uniqueName, MachineMeta& meta, MachineData& data)
    {
        int nNozzlePos = uniqueName.lastIndexOf("_");
        QString machineName = uniqueName;
        machineName.truncate(nNozzlePos);
        QString nozzlesString = uniqueName.mid(nNozzlePos + 1);
        if (nNozzlePos == -1 || machineName.isEmpty() || nozzlesString.isEmpty())
        {
            qWarning() << QString("ParameterManager::_checkUniqueName error format. [%1]").arg(uniqueName);
            return false;
        }

        bool find = false;
        for (const MachineMeta& machineMeta : m_machineMetas)
        {
            if (machineMeta.baseName == machineName)
            {
                meta = machineMeta;
                meta.extruderCount = machineMeta.extruderCount;
                meta.supportExtruderDiameters = machineMeta.supportExtruderDiameters;
                meta.supportMaterialTypes = machineMeta.supportMaterialTypes;
                meta.supportMaterialDiameters = machineMeta.supportMaterialDiameters;
                meta.supportMaterialNames = machineMeta.supportMaterialNames;

                find = true;
                break;
            }
        }

        if (!find)
        {
            qWarning() << QString("ParameterManager::_checkUniqueName error baseName. [%1]").arg(machineName);
            return false;
        }

        QStringList nozzles = nozzlesString.split("-");
        if (nozzles.count() != meta.extruderCount)
        {
            qWarning() << QString("ParameterManager::_checkUniqueName error extruder mismatch. [%1]").arg(nozzlesString);
            return false;
        }

        bool parseSuccess = true;
        data.extruderDiameters.clear();
        for (int i = 0; i < nozzles.count(); ++i)
        {
            bool success = true;
            float diameter = nozzles.at(i).toFloat(&success);
            if (!success)
            {
                qWarning() << QString("ParameterManager::_checkUniqueName error extruder diameter parse failed. [%1]").arg(nozzles.at(i));
                parseSuccess = false;
                break;
            }
            data.extruderDiameters.append(diameter);
        }
        if (!parseSuccess)
            return false;

        return true;
    }

    QString ParameterManager::_uniqueNameFromBaseNameNozzle(const QString& baseName, const QString& nozzle)
    {
        if (baseName.isEmpty())
            return QString();
        return QString("%1_%2").arg(baseName).arg(nozzle);
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
        if(index>=0)
        {
            m_currentMachine->setCurrentProfile(index);
        }
        else {
            m_currentMachine->setCurrentProfile(0);
        }

    }
    void ParameterManager::setExtrudersMaterial(int iExtruder, const QString& material)
    {
        m_currentMachine->setExtruderMaterial(iExtruder, material);
    }
    QString  ParameterManager::currentProfile()
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
                //PrintExtruder* extruder = qobject_cast<PrintExtruder*>(m_currentMachine->extruderObject(i));
                //extruder->materialName();
            }
        }
        return extruders;
    }
    QStringList ParameterManager::currentMaterials()
    {
        QStringList materials;
        if (m_currentMachine)
        {
            for (int i = 0; i < m_currentMachine->extruderCount(); i++)
            {
                PrintExtruder* extruder = qobject_cast<PrintExtruder*>(m_currentMachine->extruderObject(i));
                materials.append(extruder->materialName());
            }
        }
        return materials;
    }

}
