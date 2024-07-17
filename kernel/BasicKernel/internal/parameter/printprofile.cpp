#include "printprofile.h"
#include <QtCore/QDebug>
#include<QtQml/QQmlEngine>
#include <QFile>

#include "internal/parameter/parameterpath.h"
#include "internal/parameter/printmachine.h"
#include "internal/parameter/printextruder.h"
#include "internal/parameter/printmaterial.h"
#include "internal/models/parameterdatamodel.h"
namespace creative_kernel
{
    PrintProfile::PrintProfile(const QString& machineName, const QString& fileName, QObject* parent, bool isDefault)
        : ParameterBase(parent)
        , m_profileFileName(fileName)
        , m_machineName(machineName)
    {
        m_isDefault = isDefault;
        generateName(m_profileFileName);
    }

    PrintProfile::~PrintProfile()
    {
    }

    void PrintProfile::added()
    {
        if (m_isDefault)
        {
            us::USettings* s = createDefaultProfileSettings(m_machineName, m_profileFileName);
            setSettings(s);
            setUserSettings(createDefaultProfileCoverSettings(m_machineName, m_profileFileName, getCurrentMaterialName()));
        }
        else
        {
            us::USettings* s = createUserProfileSettings(m_machineName, m_profileFileName);
            setSettings(s);
            setUserSettings(createUserProfileCoverSettings(m_machineName, m_profileFileName, getCurrentMaterialName()));
        }

    }

    void PrintProfile::removed()
    {
        removeUserProfileFile(m_machineName, m_profileFileName, getCurrentMaterialName());
    }

    void PrintProfile::setName(const QString& newName)
    {
        m_name = newName;
        emit nameChanged();
    }

    QString PrintProfile::name()
    {
        return m_name;
    }

    QString PrintProfile::uniqueName()
    {
        return m_name;
    }

    void PrintProfile::changeUserFileName(const QString& newName)
    {
        PrintMachine* pm = qobject_cast<PrintMachine*>(parent());
        QString pmName = pm->uniqueName();

        QString path = userProfileFile(pmName, name());
        QString coverPath = userProfileCoverFile(pmName, name());

        QString newPath = userProfileFile(pmName, newName);
        QString newCoverPath = userProfileCoverFile(pmName, newName);
        
        QFile file(path);
        if (file.isOpen())
        {
            file.close();
        }
        bool res = file.rename(newPath);

        QFile file1(coverPath);
        if (file1.isOpen())
        {
            file1.close();
        }
            
        res = file1.rename(newCoverPath);

        generateName(newName);
    }

    QString PrintProfile::profileFileName()
    {
        return m_profileFileName;
    }

    bool PrintProfile::isDefault()
    {
        return m_isDefault;
    }

    void PrintProfile::enableChanged(const QString& key, bool enabled)
    {
        if (key == "enable_prime_tower")
        {
            emit enablePrimeTowerChanged(enabled);
        }
        else if (key == "enable_support")
        {
            emit enableSupportChanged(enabled);
        }
    }

    void PrintProfile::strChanged(const QString& key, const QString& str)
    {
        if (key == "prime_volume")
        {
            emit currentProcessPrimeVolumeChanged(str.toFloat());
        }
        else if (key == "prime_tower_width")
        {
            emit currentProcessPrimeTowerWidthChanged(str.toFloat());
        }
        else if (key == "layer_height")
        {
            emit currentProcessLayerHeightChanged(str.toFloat());
        }
        else if (key == "support_filament")
        {
            emit currentProcessSupportFilamentChanged(str.toInt());
        }
        else if (key == "support_interface_filament")
        {
            emit currentProcessSupportInterfaceFilamentChanged(str.toInt());
        }
        else if (key == "print_sequence")
        {
            emit printSequenceChanged(str);
        }
    }

    void PrintProfile::save()
    {
        saveSetting(userProfileCoverFile(m_machineName, m_profileFileName, getCurrentMaterialName()));
    }

    void PrintProfile::cancel()
    {

    }

   void PrintProfile::reset()
    {
       if (m_BasicDataModel)
       {
           QList<QString> keys;
           QHash<QString, us::USetting* >::const_iterator it = m_user_settings->settings().constBegin();
           while (it != m_user_settings->settings().constEnd())
           {
               keys.append(it.key());
               it++;
           }
           for (int i = 0; i < keys.size(); i++)
           {
               //m_profileParameterModel->resetValue(keys[i]);
               m_BasicDataModel->resetValue(keys[i]);
           }
       }
       ParameterBase::reset();
    }

   QStringList PrintProfile::processDiffKeys(QObject* obj)
   {
       PrintProfile* profile = qobject_cast<PrintProfile*>(obj);
       QStringList diffKeys = this->compareSettings(profile);
       return diffKeys;
   }

   QString PrintProfile::getCurrentMaterialName() const
   {
       PrintMachine* machine = qobject_cast<PrintMachine*>(this->parent());
       if (machine)
       {
           if (machine->extruderCount() == 1)
           {
               return machine->extruderMaterialName(0);
           }
       }
       return "";
   }

   void PrintProfile::generateName(const QString& fileName)
   {
       int res = fileName.indexOf(".json");
       QString tempName;
       if (res)
       {
           tempName = fileName.mid(0, m_profileFileName.indexOf(".json"));
       }
       else
       {
           tempName = fileName;
       }
       setName(tempName);
   }

    QAbstractListModel* PrintProfile::profileParameterModel(const bool& isSingleExtruder)
    {
        if (isSingleExtruder)
        {
            setUserSettings(createDefaultProfileCoverSettings(m_machineName, m_profileFileName, getCurrentMaterialName()));
            getDataModel()->setModifyedSettings(m_user_settings);
        }

        return nullptr;
    }

    QObject* PrintProfile::basicDataModel()
    {
        return getDataModel();
    }

    ParameterDataModel* PrintProfile::getDataModel()
    {
        if (!m_BasicDataModel)
        {
            m_BasicDataModel = new ParameterDataModel(m_settings, m_user_settings, this);
            QQmlEngine::setObjectOwnership(m_BasicDataModel, QQmlEngine::QQmlEngine::CppOwnership);
        }
        return m_BasicDataModel;
    }
}
