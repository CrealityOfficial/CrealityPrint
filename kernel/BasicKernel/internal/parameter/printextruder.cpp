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
            m_materialName = machine->materialsNameList().at(0);
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

    void PrintExtruder::setColor(const QColor& color) {
        if (m_materialColor != color) {
            m_materialColor = color;
            colorChanged();
        }
    }

    QColor PrintExtruder::color() const {
        return m_materialColor;
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
        if (machine)
        {
            return machine->materialsNameList().indexOf(m_materialName);
        }
        return 0;
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
                m_materialName = materialList.at(0);
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
