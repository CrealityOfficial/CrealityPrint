#ifndef CREATIVE_KERNEL_MACHINEINTERFACE_1592732397175_H
#define CREATIVE_KERNEL_MACHINEINTERFACE_1592732397175_H
#include "basickernelexport.h"
#include <QtCore/QStringList>
#include <QtCore/QSharedPointer>
#include <QColor>
#include "us/usettings.h"
#include "data/header.h"

namespace creative_kernel
{
    BASIC_KERNEL_API QStringList machinesList();
    BASIC_KERNEL_API void setCurrentMachine(const QString& machine);
    BASIC_KERNEL_API void setCurrentProfile(const QString& profile);
    BASIC_KERNEL_API void addMachine(const QString& machine);
    BASIC_KERNEL_API void removeMachine(const QString& machine);
    BASIC_KERNEL_API bool checkMachineExist(const QString& machine);
    BASIC_KERNEL_API void setCurrentProfile(const QString& profile);
    BASIC_KERNEL_API void setExtrudersMaterial(int iExtruder, const QString& material);
    BASIC_KERNEL_API QString currentMachineName();
    BASIC_KERNEL_API QString currentProfile();
    BASIC_KERNEL_API QStringList currentProfiles();
    BASIC_KERNEL_API QStringList currentExtruders();
    BASIC_KERNEL_API QStringList currentMaterials();
    BASIC_KERNEL_API QString currentMaterialsType(int index);
    BASIC_KERNEL_API std::vector<trimesh::vec> currentColors();
    BASIC_KERNEL_API bool currentMachineIsBelt();
    BASIC_KERNEL_API bool currentMachineCenterIsZero();
    BASIC_KERNEL_API int currentMachineExtruderCount();
    BASIC_KERNEL_API QList<QColor> currentMachineExtruderColorList();
    BASIC_KERNEL_API bool currentMachineSupportExportImage();
    BASIC_KERNEL_API bool currentMachineIsBbl();
    BASIC_KERNEL_API int currentPlateIndex();

    BASIC_KERNEL_API float currentProcessLayerHeight();
    BASIC_KERNEL_API float currentMinLayerHeight();
    BASIC_KERNEL_API float currentMaxLayerHeight();

    BASIC_KERNEL_API const QList<MaterialData>& materialMetas();

    BASIC_KERNEL_API QSharedPointer<us::USettings> createCurrentGlobalSettings();
    BASIC_KERNEL_API us::USettings* currentGlobalUserSettings();

    BASIC_KERNEL_API float nozzle_volume();

    BASIC_KERNEL_API QList<QSharedPointer<us::USettings>> createCurrentExtruderSettings();
    BASIC_KERNEL_API QSharedPointer<us::USettings> createDefaultMachineSetting(const QString& baseMachineName);
    BASIC_KERNEL_API bool settingsDirty();
    BASIC_KERNEL_API void clearSettingsDirty();
    BASIC_KERNEL_API QSharedPointer<us::USettings> createMachineSettings(QString machine_unique_name);
    BASIC_KERNEL_API QString getMachineCodeName(QString codeName);
    BASIC_KERNEL_API QString getMachineUniqueName(QString codeName);
    BASIC_KERNEL_API QString currentMachineShowName();
    BASIC_KERNEL_API bool exportAllCurrentSettings(const QString& file_path);
    BASIC_KERNEL_API void loadParamFromProject(const QByteArray& data);
}
#endif // CREATIVE_KERNEL_MACHINEINTERFACE_1592732397175_H
