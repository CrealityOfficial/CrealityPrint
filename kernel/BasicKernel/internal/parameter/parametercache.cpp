#include "parametercache.h"
#include <QtCore/QSettings>

namespace creative_kernel
{
    QStringList readCacheMachineUniqueNames()
	{
        QStringList machineUniqueNames;
        QSettings settings;
        settings.beginGroup("PresetMachines");
        machineUniqueNames = settings.childGroups();
        settings.endGroup();
        return machineUniqueNames;
	}

	void removeCacheMachine(const QString& machineUniqueNames)
	{
        QSettings settings;
        settings.beginGroup("PresetMachines");
        settings.remove(machineUniqueNames);
        settings.endGroup();
    }

    int readCacheCurrentMachineIndex()
    {
        QSettings setting;
        setting.beginGroup("PresetMachines");
        int index = setting.value("machine_curren_index", "-1").toInt();
        setting.endGroup();
        return index;
    }

    void writeCacheCurrentMachineIndex(int index)
    {
        QSettings setting;
        setting.beginGroup("PresetMachines");
        setting.setValue("machine_curren_index", index);
        setting.endGroup();
    }

    //int readCacheCurrentProfile(const QString& machineName)
    //{
    //    QSettings settings;
    //    settings.beginGroup("PresetMachines");
    //    settings.beginGroup(machineName);
    //    int index = settings.value("CurrentProfile", -1).toInt();
    //    settings.endGroup();
    //    settings.endGroup();
    //    return index;
    //}

    QString readCacheCurrentProfile(const QString& machineName)
    {
        QSettings settings;
        settings.beginGroup("PresetMachines");
        settings.beginGroup(machineName);
        QString profileName = settings.value("CurrentProfile", -1).toString();
        settings.endGroup();
        settings.endGroup();
        return profileName;
    }

    void writeCacheCurrentProfile(const QString& machineName, int index)
    {
        QSettings setting;
        setting.beginGroup("PresetMachines");
        setting.beginGroup(machineName);
        setting.setValue("CurrentProfile", index);
        setting.endGroup();
        setting.endGroup();
    }

    void writeCacheCurrentProfile(const QString& machineName, const QString& profileName)
    {
        QSettings setting;
        setting.beginGroup("PresetMachines");
        setting.beginGroup(machineName);
        setting.setValue("CurrentProfile", profileName);
        setting.endGroup();
        setting.endGroup();
    }

    QStringList readCacheSelectMaterials(const QString& machineName, const int& index)
    {
        QStringList materialNames;
        QSettings settings;
        settings.beginGroup("PresetMachines");
        settings.beginGroup(machineName);
        settings.beginGroup("Extruders");
        settings.beginGroup(std::to_string(index).c_str());
        int size = settings.beginReadArray("PresetMaterials");
        QStringList keysList = settings.allKeys();
        for (int i = 0; i < size; ++i) {
            settings.setArrayIndex(i);
            QString materialName = settings.value("name").toString();
            if (!materialName.isEmpty() && !materialNames.contains(materialName))
                materialNames.append(materialName);
        }
        
        settings.endArray();
        settings.endGroup();
        settings.endGroup();
        settings.endGroup();
        return materialNames;
    }

    void writeCacheSelectMaterials(const QString& machineName, const QStringList& materialNames, const int& index)
    {
        QSettings settings;

        settings.beginGroup("PresetMachines");
        settings.beginGroup(machineName);
        settings.beginGroup("Extruders");
        settings.beginGroup(std::to_string(index).c_str());
        settings.remove("PresetMaterials");
        settings.beginWriteArray("PresetMaterials");
        for (int i = 0; i < materialNames.size(); ++i) {
            settings.setArrayIndex(i);
            settings.setValue("name", materialNames[i]);
        }
        
        settings.endArray();
        settings.endGroup();
        settings.endGroup();
        settings.endGroup();
        settings.endGroup();
    }

    bool removeMachineMaterials(const QString& machineName, const QStringList& materials, const int& index)
    {
        QSettings settings;
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
        QSettings settings;
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
        QSettings settings;
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
        QSettings settings;
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
}