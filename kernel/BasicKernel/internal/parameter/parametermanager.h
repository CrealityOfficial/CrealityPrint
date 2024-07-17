#ifndef _NULLSPACE_PARAMETERSETTING_15916733167626_H
#define _NULLSPACE_PARAMETERSETTING_15916733167626_H

#include <set>

#include <QtCore/QObject>

#include "external/us/usettings.h"
#include "external/data/interface.h"

#include "internal/data/cusaddprintermodel.h"
#include "internal/parameter/materialcenter.h"
#include "cxcloud/model/profile_model.h"

namespace creative_kernel
{
    class PrintMaterial;
    class PrintMachine;
    class UserMachineModel;
    class PrintMachineListModel;
    class ParameterManager : public QObject
    {
        Q_OBJECT;
        Q_PROPERTY(int curMachineIndex READ curMachineIndex NOTIFY curMachineIndexChanged);
        Q_PROPERTY(int machinesCount READ machinesCount NOTIFY machinesCountChanged);
        Q_PROPERTY(int functionType READ functionType NOTIFY functionTypeChanged);
        Q_PROPERTY(QObject* currentMachineObject READ currentMachineObject NOTIFY curMachineIndexChanged);
        Q_PROPERTY(QStringList machineNameList READ machineNameList NOTIFY machineNameListChanged);
        Q_PROPERTY(QStringList userMachineNameList READ userMachineNameList NOTIFY userMachineNameListChanged);
        Q_PROPERTY(QStringList defaultMachineNameList READ defaultMachineNameList NOTIFY defaultMachineNameListChanged);
        Q_PROPERTY(QObject* machineListModel READ machineListModelObject CONSTANT);
        Q_PROPERTY(QObject* addPrinterListModel READ getAddPrinterListModelObject CONSTANT);
        Q_PROPERTY(QObject* materialCenter READ getMaterialCenterObject CONSTANT);
        Q_PROPERTY(int currBedTypeIndex READ currBedTypeIndex NOTIFY currBedTypeChanged);


    public:
        ParameterManager(QObject *parent = nullptr);
        virtual ~ParameterManager();

        void initialize();
        void reinitialize();

        int functionType();
        int machinesCount();
        int curMachineIndex();
        QStringList machineNameList();
        QStringList userMachineNameList();
        QStringList defaultMachineNameList();
        QObject* machineListModelObject();
        PrintMachineListModel* machineListModel();

        static PrintMachine* currentMachine_s();
        QStringList machineCurrentList();
        Q_INVOKABLE QString machineNozzleSelectList(const QString machineName);
        Q_INVOKABLE QStringList machinesNames();
        Q_INVOKABLE void setFunctionType(int type);
        Q_INVOKABLE void addMachine(const QString &modelMachineName, const QString &nozzle = QString());
        // 添加用户自定义机型
        Q_INVOKABLE QStringList materialTemplates();
        Q_INVOKABLE QStringList processTemplates();
        Q_INVOKABLE QObject* userMachineModel();
        Q_INVOKABLE void abandonMachine();
        Q_INVOKABLE void saveMachine();
        Q_INVOKABLE int saveMachineAs(const QString& modelMachineName, const QString& userMachineName);
        Q_INVOKABLE QString modifiedMachineName();
        Q_INVOKABLE QString modifiedMachineFullName();
        Q_INVOKABLE bool isMachineModified();
        Q_INVOKABLE void reloadAffectedMachines(QObject* machineTemplate, const int& modifiedType, const QString& modifiedName);
        Q_INVOKABLE void addUserMachine(QObject* machineTemplate, const QStringList& materials = {}, const QStringList& processes = {});
        Q_INVOKABLE void importUserMachine(const QString &srcFilePath, const QString &userMachineName, const QString &machineName, const QString &nozzle = QString());

        Q_INVOKABLE bool isMachineInFactory(const QString& name);
        Q_INVOKABLE bool isMachineInUser(const QString& name);
        Q_INVOKABLE bool isMachineNameRepeat(const QString machineName);
        Q_INVOKABLE bool checkMachineExist(const QString &baseName, const QString &nozzle);
        Q_INVOKABLE bool checkMachineExist(const QString &uniqueName) { return _checkMachineExist(uniqueName); }
        Q_INVOKABLE void setCurrentMachine(int index, bool needEmit = true);
        Q_INVOKABLE void removeMachine(QObject *machine);
        PrintMachine* machine(int index);
        Q_INVOKABLE QObject *machineObject(int index);
        Q_INVOKABLE QObject *machineObject(const QString& machineName);
        Q_INVOKABLE QObject *getMachineObjectByName(QString machineName, float extruderDiameters);
        Q_INVOKABLE QString currentMachineName();
        Q_INVOKABLE QString currentMachineShowName();
        PrintMachine* currentMachine() const;
        Q_INVOKABLE void applyCurrentMachine();

        Q_INVOKABLE bool currentMachineIsBelt();
        Q_INVOKABLE bool currentMachineCenterIsZero();
        Q_INVOKABLE int curMachineType(); ////1: fmd, 2: fdm-laser, 3: fdm-laser-cnc
        Q_INVOKABLE int curExtruderCount();
        Q_INVOKABLE QList<QColor> curExtrudersColor();
        Q_INVOKABLE bool currentMachineSupportExportImage();
        bool currentMachineIsBbl();
        Q_INVOKABLE QObject *findMachine(const QString &machineName);

        Q_INVOKABLE QStringList extruderSupportDiameters(const QString &machineBaseName, int index);

        Q_INVOKABLE QString getFileNameByPath(const QString &path);
        Q_INVOKABLE QString getMachineName(const QString &path);
        Q_INVOKABLE QString getMachineCodeName(const QString& machineName);
        Q_INVOKABLE QString getMachineNameFromCode(const QString& codeName);
        Q_INVOKABLE QString getUniqueNameFromCode(const QString& codeName);

        Q_INVOKABLE int getMachineExtruderCount(const QString &path);
        // 打印机配置导入
        Q_INVOKABLE void importPrinterFromQml(const QString &urlString);
        Q_INVOKABLE void importPrinterFromOrca(MachineData &meta);
        Q_INVOKABLE bool checkVisSceneSensitiveParam(const QString &key) const;
        Q_INVOKABLE void syncCurParamToVisScene() const;
        void saveCurrentProjectSettings(const QString& filename);
        void handleLegacy(std::string& opt_key, std::string& value);
        void loadProjectSettings(const QString& filename);
        QMap<int,int> loadModelsSettings(const QString& filename);
        bool exportAllCurrentSettings(const QString& file_path);
        void loadParamFromProject(const QString& zip_file_name);

        Q_INVOKABLE void setProfileAdvanced(bool checked);
        Q_INVOKABLE bool getProfileAdvanced();
        Q_INVOKABLE void setMachineAdvanced(bool checked);
        Q_INVOKABLE bool getMachineAdvanced();

        Q_INVOKABLE QString getCodeNameByBaseName(const QString& baseName);
        Q_INVOKABLE QString getBaseNameByUniqueName(const QString& uniqueName);
        Q_INVOKABLE QString getNameByUniqueName(const QString& uniqueName);
        Q_INVOKABLE QString getModifyModelDataFromType(const QString type);
        QObject* currentMachineObject();
        bool settingsDirty();
        void clearSettingsDirty();
        QSharedPointer<us::USettings> createCurrentGlobalSettings();
        QList<QSharedPointer<us::USettings>> createCurrentExtruderSettings();
        QStringList currentProfiles();
        QString currentProfile();
        QStringList currentExtruders();
        QStringList currentMaterials();
        QString currentMaterialsType(int index);
        void setCurrentProfile(QString profile);
        void setExtrudersMaterial(int iExtruder, const QString &material);

        std::vector<trimesh::vec> currentColors();

        Q_INVOKABLE QStringList currrentBedKeys();
        Q_INVOKABLE QVariantList currrentBedTypes();

        Q_INVOKABLE float currentMachineWidth();
        Q_INVOKABLE float currentMachinedepth();
        Q_INVOKABLE float currentMachineheight();

        float extruder_clearance_radius();
        QString printSequence();
        Q_INVOKABLE QString currBedType();
        Q_INVOKABLE int currBedTypeIndex();
        QString firstLayerPrintSequence();
        int currentPlateIndex();

        void setCurrentPlateIndex(const int& index);
        void setPrintSequence(const QString& print_sequence);
        Q_INVOKABLE void setCurrBedType(const QString& curr_bed_type);
        void setFirstLayerPrintSequence(const QString& first_layer_print_sequence);
        void resetPlateSettings();

        bool enablePrimeTower();
        float currentProcessLayerHeight();
        float currentProcessPrimeVolume();
        float currentProcessPrimeTowerWidth();
        bool enableSupport();
        int currentProcessSupportFilament();
        int currentProcessSupportInterfaceFilament();
        float extruderClearanceRadius();
        float extruderClearanceHeightToLid();
        float extruderClearanceHeightToRod();
        Q_INVOKABLE bool isAnyModified();
        Q_INVOKABLE void abandonAll();
        std::vector<trimesh::vec3> extruderClearanceContour();

        Q_INVOKABLE bool checkUniqueName(const QString& uniqueName);

        Q_SIGNAL void machineListChange();

        // 查询两个machine具有不同的value的keys
        Q_INVOKABLE QStringList printDiffKeys(int printerIndex1, int printerIndex2);
        QStringList printDiffKeys(const QString printerName1, const QString printerName2);

        /// @param parameter_base Edited ParameterBase*, or new ParameterBase* in current context.
        /// @param key Key of edited paramter, or empty if only changed the ParameterBase.
        Q_SIGNAL void parameterEdited(QObject* parameter_base, const QString& key);

    public:
        CusAddPrinterModel* getAddPrinterListModel() const;
        QObject* getAddPrinterListModelObject() const;

        MaterialCenter* getMaterialCenter() const;
        QObject* getMaterialCenterObject() const;

    public: // about parameter update @see cxcloud::ProfileService and ParameterUpdateDialog
        Q_SLOT void onCheckUpdateableFinished(bool updateable, bool overrideable);
        Q_SLOT void onUpdateOfficalPrinterFinished(const QString& uid, bool successed);
        Q_SLOT void onOverrideOfficalProfileFinished(bool successed);
        Q_SLOT void onUpdateProfileLanguageFinished(bool successed);
        Q_INVOKABLE void requestUpdateableMessage();

    protected:
        void fileToMachineAndExtruder(const QString &sourceFilePath, const QString &machinePath, const QString &extruderPath);
        void _addMachine(PrintMachine *machine);
        void _addImportMachine(PrintMachine *machine);
        void _addMachineFromUniqueName(const QString &uniqueName);
        void _setCurrentMachine(PrintMachine *machine, bool needEmit = true);
        void _removeMachine(PrintMachine *machine);
        bool _checkMachineExist(const QString &uniqueName);
        PrintMachine *_createMachine(const QString& uniqueName);
        // 用户新增的机型相关的Mate写入到json
        bool addMachineMeta(const MachineData &machineMeta);
        void _removeMachine(const QString &machine);
        void _cacheCurrentMachineIndex();
        QStringList _machineNames(const QList<PrintMachine *> &machines);
        QStringList _machineUniqueNames(const QList<PrintMachine *> &machines);

        PrintMachine *_findMachine(const QString &uniqueName);
        QString _uniqueNameFromBaseNameNozzle(const QString &baseName, const QString &nozzle);
    signals:
        void curMachineIndexChanged();
        void machinesCountChanged();
        void functionTypeChanged();
        void machinesNamesChanged();

        void machineNameListChanged();
        void userMachineNameListChanged();
        void defaultMachineNameListChanged();
        void machineCurrentListChanged();
        void currentColorsChanged();
        void enablePrimeTowerChanged(bool enabled);
        void currentProcessLayerHeightChanged(float height);
        void currentProcessPrimeVolumeChanged(float volume);
        void currentProcessPrimeTowerWidthChanged(float width);
        void enableSupportChanged(bool enabled);
        void currentProcessSupportFilamentChanged(int index);
        void currentProcessSupportInterfaceFilamentChanged(int index);
        void extruderClearanceRadiusChanged(float height);
        void extruderClearanceHeightToLidChanged(float volume);
        void extruderClearanceHeightToRodChanged(float width);
        void printSequenceChanged(const QString& str);
        void currBedTypeChanged(const QString& str);

    private:
        Q_SLOT void onCurrentMachineChanged();
        Q_SLOT void onParameterEdited();

    private:
        MaterialCenter* m_materialCenter{ nullptr };
        CusAddPrinterModel* m_addPrinterModel{ nullptr };

        QList<PrintMachine *> m_machines{};
        PrintMachine *m_currentMachine{ nullptr };
        PrintMachine *m_emptyMachine{ nullptr };
        UserMachineModel* m_userMachineModel = nullptr;

        int m_functionType{ 0 };

        QList<MachineData> m_machineDatas{};
        QList<MachineData> m_userMachineDatas{};
        QStringList m_factoryMachineNames{};
        QStringList m_userMachineNames{};

        PrintMachineListModel* m_MachineListModel = nullptr;
        QList<QMetaObject::Connection> context_connections_;
    };
}

#endif // _NULLSPACE_PARAMETERSETTING_15916733167626_H
