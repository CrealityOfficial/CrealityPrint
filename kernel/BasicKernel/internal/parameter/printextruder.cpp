#include "printextruder.h"
#include "internal/parameter/printextruder.h"
#include "internal/parameter/parameterpath.h"
#include "internal/parameter/printmaterial.h"
#include "internal/parameter/printmachine.h"
#include "internal/models/profileparametermodel.h"

#include <QtCore/QFile>
#include <QtCore/QDebug>

namespace creative_kernel
{
    enum {
        name_role = Qt::UserRole + 1,
        
    };

	PrintExtruder::PrintExtruder(const QString& machineName, int index, QObject* parent)
		: ParameterBase(parent)
		, m_index(index)
		, m_machineName(machineName)
	{

	}

	PrintExtruder::~PrintExtruder()
	{

	}

	void PrintExtruder::added()
	{
        PrintMachine* printMachine = qobject_cast<PrintMachine*>(this->parent());
        bool isFromUserMachine = printMachine->isFromUserMachine();
        if (isFromUserMachine)
        {
            setSettings(createExtruderSettings(m_machineName, m_index));
            setUserSettings(createUserExtruderSettings(m_machineName, m_index));
        }
        else
        {
            setSettings(createExtruderSettings(m_machineName, m_index));
            setUserSettings(createUserExtruderSettings(m_machineName, m_index));
        }
     
    }

	void PrintExtruder::removed()
	{
        removeUserExtuderFile(m_machineName, m_index);
	}

    void PrintExtruder::setMaterial(const QString& materialName)
    {
        if (m_materialName == materialName)
            return;

        m_materialName = materialName;
        emit materialIndexChanged();
    }
    int PrintExtruder::materialIndex()
    {
        PrintMachine* machine = qobject_cast<PrintMachine*>(parent());
        if (machine)
        {
            return machine->materialsNameList().indexOf(m_materialName);
        }
        return 0;
    }

    void PrintExtruder::setMaterial(int materialIndex)
    {

    }

    QAbstractListModel* PrintExtruder::extruderParameterModel(const QString& category, bool professional)
    {
        ProfileParameterModel* model = nullptr;
        QString key = category + (professional ? "-advance" : "");
        if (m_extruderParameterModels.find(key) == m_extruderParameterModels.end()) {
            model = new ProfileParameterModel(m_settings, this);
            m_extruderParameterModels[key] = model;
        }
        else
        {
            model = m_extruderParameterModels[key];
        }
        model->setExtruderCategory(category, professional);
        model->applyUserSetting(m_user_settings);
        return model;
    }

    QString PrintExtruder::materialName()
    {
        return m_materialName;
    }

    void PrintExtruder::save()
    {
        saveSetting(userExtruderFile(m_machineName, m_index));
    }

    void PrintExtruder::cancel()
    {
    }

    void PrintExtruder::reset()
    {
        /*QString key = materialName + (professional ? "-advance" : "");
        if (m_extruderParameterModels)
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
                m_materialParameterModel->resetValue(keys[i]);
            }
        }
        save();*/
    }

    void  PrintExtruder::reset(const QString& category, bool professional/* = false*/)
    {
        QString key = category + (professional ? "-advance" : "");
        ProfileParameterModel* model = m_extruderParameterModels[key];

        if (model)
        {
            QList<QString> keys;
            QHash<QString, us::USetting* >::const_iterator it = m_user_settings->settings().constBegin();
            while (it != m_user_settings->settings().constEnd())
            {
                keys.append(it.key());
                it++;
            }
            for (const auto& key : keys)
            {
                if (model->hasKeyInFilter(key))
                {
                    model->resetValue(key);
                }
            }
        }
        save();
    }
}