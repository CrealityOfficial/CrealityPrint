#include "parametercache.h"

#include <cxcloud/tool/settings.h>

namespace creative_kernel
{
    QStringList readCacheMachineUniqueNames()
	{
        QStringList machineUniqueNames;
        cxcloud::VersionServerSettings settings;
        settings.beginGroup("PresetMachines");
        machineUniqueNames = settings.childGroups();
        settings.endGroup();
        return machineUniqueNames;
	}

    int readCacheMachineExtruders(const QString& machineUniqueNames, std::vector<QColor>& extruderColors, std::vector<bool>& extruderPhysicals)
    {
        int extruderCounts = 0;
        cxcloud::VersionServerSettings settings;
        settings.beginGroup("PresetMachines");
        settings.beginGroup(machineUniqueNames);
        settings.beginGroup("Extruders");
        for (int extruder = 0; extruder < settings.childGroups().size(); ++extruder)
        {
            settings.beginGroup(QString("%1").arg(extruder));
            auto color = settings.value("Color", 0x3DDF56).toUInt();
            extruderColors.push_back(color);
            extruderPhysicals.push_back(settings.value("IsPhysical", 1).toBool());
            settings.endGroup();
            ++extruderCounts;
        }
        settings.endGroup();
        settings.endGroup();
        settings.endGroup();
        if (extruderCounts == 0)
        {
            extruderCounts = 1;
        }

        if (extruderCounts > 16)
            extruderCounts = 16;
        return extruderCounts;
    }

    void writeCacheMachineExtruder(const QString& machineUniqueNames, const QColor& extruderColor, const bool& extruderPhysical)
    {
        cxcloud::VersionServerSettings settings;
        settings.beginGroup("PresetMachines");
        settings.beginGroup(machineUniqueNames);
        settings.beginGroup("Extruders");
        settings.beginGroup(QString::number(settings.childGroups().size()));
        settings.setValue("Color", extruderColor.rgba64().toArgb32());
        settings.setValue("IsPhysical", extruderPhysical);
        settings.endGroup();
        settings.endGroup();
        settings.endGroup();
        settings.endGroup();
    }

    void modifyCacheMachineExtruder(const QString& machineUniqueNames, const int& index, const QColor& extruderColor)
    {
        cxcloud::VersionServerSettings settings;
        settings.beginGroup("PresetMachines");
        settings.beginGroup(machineUniqueNames);
        settings.beginGroup("Extruders");
        settings.beginGroup(QString::number(index));
        settings.setValue("Color", extruderColor.rgba64().toArgb32());
        settings.endGroup();
        settings.endGroup();
        settings.endGroup();
        settings.endGroup();
    }

    void removeCacheMachineExtruder(const QString& machineUniqueNames)
    {
        cxcloud::VersionServerSettings settings;
        settings.beginGroup("PresetMachines");
        settings.beginGroup(machineUniqueNames);
        settings.beginGroup("Extruders");
        settings.remove(QString::number(settings.childGroups().size()-1));
        settings.endGroup();
        settings.endGroup();
        settings.endGroup();
    }

	void removeCacheMachine(const QString& machineUniqueNames)
	{
        cxcloud::VersionServerSettings settings;
        settings.beginGroup("PresetMachines");
        settings.remove(machineUniqueNames);
        settings.endGroup();
    }

    int readCacheCurrentMachineIndex()
    {
        cxcloud::VersionServerSettings setting;
        setting.beginGroup("PresetMachines");
        int index = setting.value("machine_curren_index", "-1").toInt();
        setting.endGroup();
        return index;
    }

    void writeCacheCurrentMachineIndex(int index)
    {
        cxcloud::VersionServerSettings setting;
        setting.beginGroup("PresetMachines");
        setting.setValue("machine_curren_index", index);
        setting.endGroup();
    }

    //int readCacheCurrentProfile(const QString& machineName)
    //{
    //    cxcloud::VersionServerSettings settings;
    //    settings.beginGroup("PresetMachines");
    //    settings.beginGroup(machineName);
    //    int index = settings.value("CurrentProfile", -1).toInt();
    //    settings.endGroup();
    //    settings.endGroup();
    //    return index;
    //}

    QString readCacheCurrentProfile(const QString& machineName)
    {
        cxcloud::VersionServerSettings settings;
        settings.beginGroup("PresetMachines");
        settings.beginGroup(machineName);
        QString profileName = settings.value("CurrentProfile", "").toString();
        settings.endGroup();
        settings.endGroup();
        return profileName;
    }

    void writeCacheCurrentProfile(const QString& machineName, int index)
    {
        cxcloud::VersionServerSettings setting;
        setting.beginGroup("PresetMachines");
        setting.beginGroup(machineName);
        setting.setValue("CurrentProfile", index);
        setting.endGroup();
        setting.endGroup();
    }

    void writeCacheCurrentProfile(const QString& machineName, const QString& profileName)
    {
        cxcloud::VersionServerSettings setting;
        setting.beginGroup("PresetMachines");
        setting.beginGroup(machineName);
        setting.setValue("CurrentProfile", profileName);
        setting.endGroup();
        setting.endGroup();
    }

    QStringList readCacheSelectMaterials(const QString& machineName)
    {
        QStringList materialNames;
        cxcloud::VersionServerSettings settings;
        settings.beginGroup("PresetMachines");
        settings.beginGroup(machineName);
        QString presetMaterials = settings.value("PresetMaterials", "").toString();
        if(presetMaterials!="")
            materialNames = presetMaterials.split(",");
        settings.endGroup();
        settings.endGroup();
        return materialNames;
    }

    void writeCacheSelectMaterials(const QString& machineName, const QStringList& materialNames)
    {
        cxcloud::VersionServerSettings settings;

        settings.beginGroup("PresetMachines");
        settings.beginGroup(machineName);
        if(materialNames.size()==0)
            settings.setValue("PresetMaterials", "");
        else
            settings.setValue("PresetMaterials", materialNames.join(","));
        settings.endGroup();
        settings.endGroup();
    }

    bool removeMachineMaterials(const QString& machineName, const QStringList& materials, const int& index)
    {
        cxcloud::VersionServerSettings settings;
        settings.beginGroup("PresetMachines");
        settings.beginGroup(machineName);
        settings.beginGroup("Extruders");
        settings.beginGroup(std::to_string(index).c_str());

        settings.beginWriteArray("PresetMaterials");
        QStringList keysList = settings.allKeys();
        for (int i = 0; i < keysList.size(); ++i)
        {
            settings.setArrayIndex(i);
            QString materialName = settings.value("name").toString();
            if (materialName == materials[0].split("_")[0])
            {
                settings.endGroup();
                settings.beginGroup("PresetMaterials");
                QStringList keys = settings.allKeys();
                settings.remove(QString("%1").arg(i+1));
                keys = settings.allKeys();
                settings.endGroup();
                settings.endGroup();
                settings.endGroup();
                return true;
            }
        }

        settings.endArray();
        settings.endGroup();
        settings.endGroup();
        settings.endGroup();
        settings.endGroup();
        return false;
    }

    bool reNameMachineMaterial(const QString& machineName, const QString& oldMaterialName, const QString& newMaterialName, const int& extruderIndex)
    {
        cxcloud::VersionServerSettings settings;
        settings.beginGroup("PresetMachines");
        settings.beginGroup(machineName);
        settings.beginGroup("Extruders");
        settings.beginGroup(std::to_string(extruderIndex).c_str());

        settings.beginWriteArray("PresetMaterials");
        QStringList keysList = settings.allKeys();
        for (int i = 0; i < keysList.size(); ++i)
        {
            settings.setArrayIndex(i);
            QString materialName = settings.value("name").toString();
            if (materialName == oldMaterialName)
            {
     /*           settings.endGroup();
                settings.beginGroup("PresetMaterials");*/
                settings.setValue("name", newMaterialName);
                settings.endArray();
                settings.endGroup();
                settings.endGroup();
                settings.endGroup();
                return true;
            }
        }

        settings.endArray();
        settings.endGroup();
        settings.endGroup();
        settings.endGroup();
        settings.endGroup();
        return false;
    }

    QString readCacheExtruderMaterialIndex(const QString& machineName, int extruderIndex)
    {
        QString indexPrefix = QString("%1").arg(extruderIndex);
        cxcloud::VersionServerSettings settings;
        settings.beginGroup("PresetMachines");
        settings.beginGroup(machineName);
        settings.beginGroup("Extruders");
        settings.beginGroup(indexPrefix);
        QString materialName = settings.value("CurrentMaterial", -1).toString();
        settings.endGroup();
        settings.endGroup();
        settings.endGroup();
        settings.endGroup();
        return materialName;
    }

    void writeCacheExtruderMaterialIndex(const QString& machineName, int extruderIndex, const QString& materialName)
    {
        QString indexPrefix = QString("%1").arg(extruderIndex);
        cxcloud::VersionServerSettings settings;
        settings.beginGroup("PresetMachines");
        settings.beginGroup(machineName);
        settings.beginGroup("Extruders");
        settings.beginGroup(indexPrefix);
        settings.setValue("CurrentMaterial", materialName);
        settings.endGroup();
        settings.endGroup();
        settings.endGroup();
        settings.endGroup();
    }
    QString readCacheCurrentBedType(const QString& machineName)
    {
        cxcloud::VersionServerSettings settings;
        settings.beginGroup("PresetMachines");
        settings.beginGroup(machineName);
        QString bedType = settings.value("CurrentBedType", "").toString();
        settings.endGroup();
        settings.endGroup();
        return bedType;
    }
    void writeCacheCurrentBedType(const QString& machineName, const QString& bedType)
    {
        cxcloud::VersionServerSettings setting;
        setting.beginGroup("PresetMachines");
        setting.beginGroup(machineName);
        setting.setValue("CurrentBedType", bedType);
        setting.endGroup();
        setting.endGroup();
    }
}
