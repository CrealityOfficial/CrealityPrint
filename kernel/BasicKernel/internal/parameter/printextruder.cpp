#include "printextruder.h"

#include <QtCore/QFile>
#include <QtCore/QSettings>
#include <QtQml/QQmlEngine>

#include <qtusercore/util/settings.h>

#include "internal/parameter/parameterpath.h"
#include "internal/parameter/printextruder.h"
#include "internal/parameter/printmachine.h"
#include "internal/parameter/printmaterial.h"

namespace creative_kernel
{
    enum {
        name_role = Qt::UserRole + 1,

    };

	PrintExtruder::PrintExtruder(const QString& machineName, int index, QObject* parent, bool isPhysical, const QColor& materialColor, us::USettings* uSettings)
		: ParameterBase(parent)
		, m_index(index)
		, m_machineName(machineName)
        , m_isPhysical(isPhysical)
        , m_materialColor(materialColor)
	{
        setSettings(uSettings);

        PrintMachine* machine = qobject_cast<PrintMachine*>(parent);
        if (machine && machine->materialsNameList().count() > 0)
        {
            PrintExtruder* pe = qobject_cast<PrintExtruder*>(machine->extruderObject(index));
            if (pe)
            {
                m_materialName = pe->materialName();
                m_MaterialIndex = pe->materialIndex();
            }
        }
	}

	PrintExtruder::~PrintExtruder()
	{

	}

	void PrintExtruder::added()
	{
        setUserSettings(createUserExtruderSettings(m_machineName, m_index));
    }

	void PrintExtruder::removed()
	{
        removeUserExtuderFile(m_machineName, m_index);
	}

    int PrintExtruder::extruderIndex()
    {
        return m_index;
    }

    void PrintExtruder::setExtruderIndex(int index)
    {
        m_index = index;
    }


    QColor PrintExtruder::color() const {
        return m_materialColor;
    }

    void PrintExtruder::setColor(const QColor& color) {
        if (m_materialColor != color) {
            m_materialColor = color;  
        }
        emit colorChanged();
    }



    QString PrintExtruder::syncId() const {
        return m_syncId;
    }

    void PrintExtruder::setSyncId(const QString& id) {
        if (m_syncId != id) {
            m_syncId = id;
        }
        emit syncIdChanged();
    }


    PrintMaterial* PrintExtruder::curMaterial()
    {
        PrintMachine* machine = qobject_cast<PrintMachine*>(parent());
        return qobject_cast<PrintMaterial*>( machine->materialObject(m_materialName));
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
        if(!machine)
            return -1;

        int materialIndex = 0;
        QVector<PrintMachine::SyncExtruderInfo> syncs = machine->syncedExtrudersInfo();
        if (syncs.size() > 0)
        {
            if (m_MaterialIndex >= 0 && m_MaterialIndex < syncs.size())
            {
                materialIndex = m_MaterialIndex;
            }
            else 
            {
                if (machine->materialsNameList().indexOf(m_materialName) == -1)
                {
                    materialIndex = syncs.size();
                }
                else {
                    materialIndex = machine->materialsNameList().indexOf(m_materialName);
                    materialIndex += syncs.size();
                }
            }
        }
        else {
            if (machine->materialsNameList().indexOf(m_materialName) == -1)
            {
                materialIndex = 0;
            }else{
				materialIndex = machine->materialsNameList().indexOf(m_materialName);
			}
        }

        return materialIndex;
    }

    void PrintExtruder::setMaterialIndex(int index)
    {
        m_MaterialIndex = index;
        emit materialIndexChanged();
    }

    ParameterDataModel* PrintExtruder::getDataModel() const {
        if (!m_dataModel) {
            auto* self = const_cast<PrintExtruder*>(this);
            m_dataModel = new ParameterDataModel(m_settings, m_user_settings, self);
            QQmlEngine::setObjectOwnership(m_dataModel, QQmlEngine::QQmlEngine::CppOwnership);
        }

        return m_dataModel;
    }

    QObject* PrintExtruder::getDataModelObject() const {
        return getDataModel();
    }

    void PrintExtruder::setMaterial(int materialIndex)
    {

    } 

    QString PrintExtruder::materialName()
    {
        PrintMachine* machine = qobject_cast<PrintMachine*>(parent());
        if (machine)
        {
            QList<QString> materialList = machine->materialsNameList();
            if (materialList.count() == 0)
                return QString();

            if (!materialList.contains(m_materialName))
            {
                QVector<PrintMachine::SyncExtruderInfo> syncs = machine->syncedExtrudersInfo();
                if (syncs.size() > 0)
                {
                    m_materialName = syncs.at(0).materialName;
                }
                else {
                    m_materialName = materialList.at(0);
                }
            }
        }

        return m_materialName;
    }

    void PrintExtruder::save()
    {
        saveSetting(userExtruderFile(m_machineName, m_index));
    }

    void PrintExtruder::reset()
    {
        if (m_dataModel != nullptr) {
            for (const auto& key : m_user_settings->settings().keys()) {
                m_dataModel->resetValue(key);
            }
        }

        ParameterBase::reset();
    }

    void PrintExtruder::cancel()
    {
    }
}
