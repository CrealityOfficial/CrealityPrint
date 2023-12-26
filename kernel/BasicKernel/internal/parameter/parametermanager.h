#ifndef _NULLSPACE_PARAMETERSETTING_15916733167626_H
#define _NULLSPACE_PARAMETERSETTING_15916733167626_H
#include <QtCore/QObject>
#include "us/usettings.h"
#include "data/interface.h"

namespace creative_kernel
{
    class PrintMaterial;
    class PrintMachine;
    class ParameterManager : public QObject
    {
        Q_OBJECT
        Q_PROPERTY(int curMachineIndex READ curMachineIndex NOTIFY curMachineIndexChanged)
        Q_PROPERTY(int machinesCount READ machinesCount NOTIFY machinesCountChanged)
        Q_PROPERTY(int functionType READ functionType NOTIFY functionTypeChanged)
        Q_PROPERTY(QStringList machineNameList READ machineNameList NOTIFY machineNameListChanged)
        Q_PROPERTY(QObject* currentMachineObj READ currentMachineObject NOTIFY curMachineIndexChanged)

    public:
        ParameterManager(QObject* parent = nullptr);
        virtual ~ParameterManager();

        void intialize();

        int functionType();
        int machinesCount();
        int curMachineIndex();
        QStringList machineNameList();

        Q_INVOKABLE QStringList machinesNames();
        Q_INVOKABLE void setFunctionType(int type);
        Q_INVOKABLE void addMachine(const QString& modelMachineName, const QString& nozzle = QString());
        //添加用户自定义机型
        Q_INVOKABLE void addUserMachine(const QString& modelMachineName, const QString& nozzle = QString(), const QString& newMachineName = QString());
        Q_INVOKABLE void importUserMachine(const QString& srcFilePath, const QString& userMachineName, const QString& machineName, const QString& nozzle = QString());

        Q_INVOKABLE bool isMachineNameRepeat(const QString machineName);
        Q_INVOKABLE bool checkMachineExist(const QString& baseName, const QString& nozzle);
        Q_INVOKABLE bool checkMachineExist(const QString& uniqueName) { return _checkMachineExist(uniqueName); }
        Q_INVOKABLE void setCurrentMachine(int index);
        Q_INVOKABLE void removeMachine(QObject* machine);
        Q_INVOKABLE QObject* machineObject(int index);
        Q_INVOKABLE QObject* currentMachineObject();
        Q_INVOKABLE QString currentMachineName();
        Q_INVOKABLE void applyCurrentMachine();

        Q_INVOKABLE bool currentMachineIsBelt();
        Q_INVOKABLE bool currentMachineCenterIsZero();
        Q_INVOKABLE int curMachineType(); ////1: fmd, 2: fdm-laser, 3: fdm-laser-cnc
        Q_INVOKABLE int curExtruderCount();
        Q_INVOKABLE bool currentMachineSupportExportImage();
        Q_INVOKABLE QObject* findMachine(const QString& machineName);

        Q_INVOKABLE QStringList extruderSupportDiameters(const QString& machineBaseName, int index);

        Q_INVOKABLE QString getFileNameByPath(const QString& path);
        Q_INVOKABLE QString getMachineName(const QString& path);
        Q_INVOKABLE int getMachineExtruderCount(const QString& path);
        //打印机配置导入
        Q_INVOKABLE void importPrinterFromQml(const QString& urlString);
        Q_INVOKABLE bool checkVisSceneSensitiveParam(const QString& key) const;
 		Q_INVOKABLE void syncCurParamToVisScene() const;
        bool settingsDirty();
        void clearSettingsDirty();
        QSharedPointer<us::USettings> createCurrentGlobalSettings();
        QList<QSharedPointer<us::USettings>> createCurrentExtruderSettings();
        QStringList currentProfiles();
        QString currentProfile();
        QStringList currentExtruders();
        QStringList currentMaterials();
        void setCurrentProfile(QString profile);
        void setExtrudersMaterial(int iExtruder, const QString& material);

        Q_INVOKABLE float currentMachineWidth();
        Q_INVOKABLE float currentMachinedepth();
        Q_INVOKABLE float currentMachineheight();
    protected:
        void fileToMachineAndExtruder(const QString& sourceFilePath, const QString &machinePath, const QString& extruderPath);
        void _addMachine(PrintMachine* machine);
        void _addImportMachine(PrintMachine* machine);
        void _addMachineFromUniqueName(const QString& uniqueName);
        void _setCurrentMachine(PrintMachine* machine);
        void _removeMachine(PrintMachine* machine);
        bool _checkMachineExist(const QString& uniqueName);
        PrintMachine* _createMachine(const QString& uniqueName, const QString& modelMachineName = QString(), bool isUser = false);
        //用户新增的机型相关的Mate写入到json
        bool addMachineMeta(const MachineMeta& machineMeta);

        bool isUserMachine(const QString& machineName);

        void _removeMachine(const QString& machine);
        void _cacheCurrentMachineIndex();
        QStringList _machineNames(const QList<PrintMachine*>& machines);
        QStringList _machineUniqueNames(const QList<PrintMachine*>& machines);

        PrintMachine* _findMachine(const QString& uniqueName);
        bool _checkUniqueName(const QString& uniqueName, MachineMeta& meta, MachineData& data);
        QString _uniqueNameFromBaseNameNozzle(const QString& baseName, const QString& nozzle);
    signals:
        void curMachineIndexChanged();
        void machinesCountChanged();
        void functionTypeChanged();
        void machinesNamesChanged();

        void machineNameListChanged();

    private:
        QList<PrintMachine*> m_machines;
        PrintMachine* m_currentMachine;
        PrintMachine* m_emptyMachine;

        int m_functionType;

        QList<MachineMeta> m_machineMetas;
        QList<MachineMeta> m_userMachineMetas;
//        QStringList m_machineNameList;
    };
}

#endif // _NULLSPACE_PARAMETERSETTING_15916733167626_H
