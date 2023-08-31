#include "machineinterface.h"
#include "kernel/kernel.h"
#include "internal/parameter/parametermanager.h"
#include "internal/parameter/parameterpath.h"
#include "internal/parameter/materialcenter.h"

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
    QStringList currentExtruders()
    {
        return getKernel()->parameterManager()->currentExtruders();
    }
    QStringList currentMaterials()
    {
        return getKernel()->parameterManager()->currentMaterials();
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

    bool currentMachineSupportExportImage()
    {
        return getKernel()->parameterManager()->currentMachineSupportExportImage();
    }

    const QList<MaterialMeta>& materialMetas()
    {
        return getKernel()->materialCenter()->materialMetas();
    }

    QSharedPointer<us::USettings> createCurrentGlobalSettings()
    {
        return getKernel()->parameterManager()->createCurrentGlobalSettings();
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
}