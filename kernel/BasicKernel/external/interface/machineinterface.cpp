#include "machineinterface.h"
#include "kernel/kernel.h"
#include "internal/parameter/parametermanager.h"
#include "internal/parameter/parameterpath.h"
#include "internal/parameter/materialcenter.h"
#include "internal/parameter/printmachine.h"
#include <QColor>
#include <QTemporaryDir>

namespace creative_kernel
{
    QStringList machinesList()
    {
        return QStringList();
    }

    void setCurrentMachine(const QString& machine)
    {
        QStringList machineList = getKernel()->parameterManager()->machineNameList();
        int index = machineList.indexOf(machine+" nozzle");
        if (index < 0)
        {
            QString sp = machine;
            sp = sp.replace("-", "+");
            index = machineList.indexOf(sp + " nozzle");
        }
        return getKernel()->parameterManager()->setCurrentMachine(index);
    }
    void setCurrentProfile(const QString& profile)
    {
        getKernel()->parameterManager()->setCurrentProfile(profile);
    }
    void setExtrudersMaterial(int iExtruder,const QString& material)
    {
        getKernel()->parameterManager()->setExtrudersMaterial(iExtruder, material);
    }
    void addMachine(const QString& machine)
    {
    }

    void removeMachine(const QString& machine)
    {
    }
    bool checkMachineExist(const QString& machine)
    {
        return getKernel()->parameterManager()->checkMachineExist(machine);
    }
    QString currentMachineName()
    {
        return getKernel()->parameterManager()->currentMachineName();
    }
    QString currentMachineShowName()
    {
        return getKernel()->parameterManager()->currentMachineShowName();
    }
    bool exportAllCurrentSettings(const QString& file_name)
    {
        return getKernel()->parameterManager()->exportAllCurrentSettings(file_name);
    }
    void loadParamFromProject(const QByteArray& data)
    {
        if (data.isEmpty())
        {
            return;
        }
        QTemporaryDir temp_dir;
        auto zip_file_name = temp_dir.filePath("param") + ".zip";

        QFile zip_file(zip_file_name);
        zip_file.open(QIODevice::WriteOnly);
        zip_file.write(data);
        zip_file.close();
        getKernel()->parameterManager()->loadParamFromProject(zip_file_name);
    }
    QStringList currentExtruders()
    {
        return getKernel()->parameterManager()->currentExtruders();
    }
    QStringList currentMaterials()
    {
        return getKernel()->parameterManager()->currentMaterials();
    }

    QString currentMaterialsType(int index)
    {
        return getKernel()->parameterManager()->currentMaterialsType(index);
    }

    std::vector<trimesh::vec> currentColors()
    {
        return getKernel()->parameterManager()->currentColors();
    }

    QString currentProfile()
    {
        return getKernel()->parameterManager()->currentProfile();
    }
    QStringList currentProfiles()
    {
        return getKernel()->parameterManager()->currentProfiles();
    }
    bool currentMachineIsBelt()
    {
        return getKernel()->parameterManager()->currentMachineIsBelt();
    }

    bool currentMachineCenterIsZero()
    {
        return  getKernel()->parameterManager() ->currentMachineCenterIsZero();
    }

    int currentMachineExtruderCount()
    {
        return getKernel()->parameterManager()->curExtruderCount();
    }

    QList<QColor> currentMachineExtruderColorList()
    {
        return getKernel()->parameterManager()->curExtrudersColor();
    }

    bool currentMachineSupportExportImage()
    {
        return getKernel()->parameterManager()->currentMachineSupportExportImage();
    }

    bool currentMachineIsBbl()
    {
        return getKernel()->parameterManager()->currentMachineIsBbl();
    }

    int currentPlateIndex()
    {
        return getKernel()->parameterManager()->currentPlateIndex();
    }

    const QList<MaterialData>& materialMetas()
    {
        return getKernel()->materialCenter()->materialMetas();
    }

    QSharedPointer<us::USettings> createCurrentGlobalSettings()
    {
        return getKernel()->parameterManager()->createCurrentGlobalSettings();
    }

    us::USettings* currentGlobalUserSettings()
    {
        PrintMachine* machine = getKernel()->parameterManager()->currentMachine();
        return machine ? machine->currentGlobalSettings() : createCurrentGlobalSettings().data();
    }

    float nozzle_volume()
    {
        PrintMachine* machine = getKernel()->parameterManager()->currentMachine();
        return machine ? machine->nozzle_volume() : 0.0f;
    }

    QList<QSharedPointer<us::USettings>> createCurrentExtruderSettings()
    {
        return getKernel()->parameterManager()->createCurrentExtruderSettings();
    }

    QSharedPointer<us::USettings> createDefaultMachineSetting(const QString& baseMachineName)
    {
        return QSharedPointer<us::USettings>(createDefaultMachineSettings(baseMachineName));
    }

    bool settingsDirty()
    {
        return getKernel()->parameterManager()->settingsDirty();
    }

    void clearSettingsDirty()
    {
        getKernel()->parameterManager()->clearSettingsDirty();
    }
    QSharedPointer<us::USettings> createMachineSettings(QString machine_unique_name)
    {
        QList<us::USettings*>* extruderSettings = new QList<us::USettings*>;
        us::USettings* uSettings = new us::USettings;
        MachineData data;
        createMachineSettings(machine_unique_name, uSettings, extruderSettings, false,&data);
        delete extruderSettings;
        return QSharedPointer<us::USettings>(uSettings);
    }
    QString getMachineCodeName(QString codeName)
    {
        return getKernel()->parameterManager()->getMachineNameFromCode(codeName);
    }
    QString getMachineUniqueName(QString codeName)
    {
        return getKernel()->parameterManager()->getUniqueNameFromCode(codeName);
    }

    float currentProcessLayerHeight()
    {
        return getKernel()->parameterManager()->currentMachine()->currentProcessLayerHeight();
    }

    float currentMinLayerHeight()
    {
        return getKernel()->parameterManager()->currentMachine()->minLayerHeight();
    }

    float currentMaxLayerHeight()
    {
        return getKernel()->parameterManager()->currentMachine()->maxLayerHeight();
    }
}
