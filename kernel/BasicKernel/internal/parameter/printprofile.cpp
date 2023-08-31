#include "printprofile.h"
#include <QtCore/QDebug>

#include "internal/parameter/parameterpath.h"
#include "internal/models/parameterprofilecategorymodel.h"
#include "internal/models/profileparametermodel.h"
#include "internal/parameter/printmachine.h"
#include "internal/parameter/printextruder.h"
#include "internal/parameter/printmaterial.h"
namespace creative_kernel
{
    PrintProfile::PrintProfile(const QString& machineName, const QString& fileName, QObject* parent)
        : ParameterBase(parent)
        , m_profileFileName(fileName)
        , m_machineName(machineName)
        , m_profileParameterModel(nullptr)
        , m_parameterProfileCategoryModel(nullptr)
    {
        m_isDefault = m_profileFileName.contains(".default");
        m_name = m_profileFileName;
        if (m_isDefault)
            m_name = m_profileFileName.mid(0, m_profileFileName.indexOf(".default"));
        
        //qDebug() << QString("PrintProfile Ctr : fileName [%1]: name [%2]").arg(m_profileFileName).arg(m_name);
    }

    PrintProfile::~PrintProfile()
    {
        if (m_profileParameterModel)
        {
            delete m_profileParameterModel;
            m_profileParameterModel = nullptr;
        }
    }

    void PrintProfile::added()
    {
        setSettings(createUserProfileSettings(m_machineName, m_profileFileName));
        setUserSettings(createUserProfileCoverSettings(m_machineName, m_profileFileName, getCurrentMaterialName()));
    }

    void PrintProfile::removed()
    {
        removeUserProfileFile(m_machineName, m_profileFileName, getCurrentMaterialName());
    }

    QString PrintProfile::name()
    {
        return m_name;
    }

    QString PrintProfile::uniqueName()
    {
        return m_name;
    }

    QString PrintProfile::profileFileName()
    {
        return m_profileFileName;
    }

    bool PrintProfile::isDefault()
    {
        return m_isDefault;
    }

    void PrintProfile::save()
    {
        saveSetting(userProfileFile(m_machineName, m_profileFileName, false, getCurrentMaterialName()));
    }

    void PrintProfile::cancel()
    {

    }

   void PrintProfile::reset()
    {
       if (m_profileParameterModel)
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
               m_profileParameterModel->resetValue(keys[i]);
           }
       }
       save();
    }

   void PrintProfile::reset(us::USettings* us)
   {
       Q_ASSERT(us);
       if (m_profileParameterModel)
       {
           QList<QString> keys;
           QHash<QString, us::USetting* >::const_iterator it = us->settings().constBegin();
           while (it != us->settings().constEnd())
           {
               keys.append(it.key());
               it++;
           }
           for (int i = 0; i < keys.size(); i++)
           {
               m_profileParameterModel->resetValue(keys[i]);
           }
       }
       save();
   }

   QMap<QString, QList<QString> > PrintProfile::getAffetecdKeys()
   {
       return m_profileParameterModel->getAffetecdKeys();
   }

   QString PrintProfile::getModelValue(QString key) const
   {
       return m_profileParameterModel->value(key);
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

    QAbstractListModel* PrintProfile::profileCategoryModel(const bool& isSingleExtruder)
    {
        if (!m_parameterProfileCategoryModel)
        {
            PrintMachine* machine = qobject_cast<PrintMachine*>(this->parent());
            if (isSingleExtruder)
            {
                PrintExtruder* extruder = qobject_cast<PrintExtruder*>((machine->extruderObject(0)));

                if (!extruder)
                    return nullptr;
                PrintMaterial* material = qobject_cast<PrintMaterial*>(machine->materialObject(extruder->materialName()));

                us::USettings* usettings = new us::USettings(this);
                usettings->merge(qobject_cast<us::USettings*>(material->settingsObject()));
                usettings->merge(qobject_cast<us::USettings*>(extruder->settingsObject()));
                usettings->merge(m_settings);
                m_parameterProfileCategoryModel = new ParameterProfileCategoryModel(usettings, this, isSingleExtruder);
            }
            else {
                m_parameterProfileCategoryModel = new ParameterProfileCategoryModel(m_settings, this, isSingleExtruder);
            }
            
            
        }
        return m_parameterProfileCategoryModel;
    }

    QAbstractListModel* PrintProfile::profileParameterModel(const bool& isSingleExtruder)
    {
        if (!m_profileParameterModel)
        {
            m_profileParameterModel = new ProfileParameterModel(m_settings, this);
        }
        if (isSingleExtruder)
        {
            m_profileParameterModel->mergeMachineSettings(0);
            m_profileParameterModel->mergeMaterialSettings(0);
            setUserSettings(createUserProfileCoverSettings(m_machineName, m_profileFileName, getCurrentMaterialName()));
        }
        m_profileParameterModel->applyUserSetting(m_user_settings);
            
        return m_profileParameterModel;
    }
}