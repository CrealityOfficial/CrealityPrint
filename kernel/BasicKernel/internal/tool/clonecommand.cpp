#include "clonecommand.h"

#include "data/modeln.h"

#include "interface/selectorinterface.h"
#include "interface/commandinterface.h"
#include "interface/visualsceneinterface.h"
#include "cxkernel/interface/uiinterface.h"
#include "interface/machineinterface.h"

#include <QtCore/QDebug>

#include "kernel/kernelui.h"

namespace creative_kernel
{
    CloneCommand::CloneCommand(QObject* parent)
        :ToolCommand(parent)
    {
        orderindex = 3;
        m_name = tr("Clone") + ": Alt + C";

        m_source = "qrc:/kernel/qml/Clone.qml";
        addUIVisualTracer(this,this);
    }

    CloneCommand::~CloneCommand()
    {
    }

    void CloneCommand::onThemeChanged(ThemeCategory category)
    {
        setDisabledIcon("qrc:/UI/photo/cToolBar/clone_dark_disable.svg");
		setEnabledIcon(category == ThemeCategory::tc_dark ? "qrc:/UI/photo/cToolBar/clone_dark_default.svg" : "qrc:/UI/photo/cToolBar/clone_light_default.svg");
		setHoveredIcon(category == ThemeCategory::tc_dark ? "qrc:/UI/photo/cToolBar/clone_dark_default.svg" : "qrc:/UI/photo/cToolBar/clone_light_default.svg");
		setPressedIcon("qrc:/UI/photo/cToolBar/clone_dark_press.svg");
    }

    void CloneCommand::onLanguageChanged(MultiLanguage language)
    {
        m_name = tr("Clone") + ": Alt + C";
    }

    bool CloneCommand::isSelect()
    {
        QList<ModelN*> selections = selectionms();
        if (selections.size() > 0)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    void CloneCommand::clone(int numb)
    {
        cloneSelections(numb);
    }

    void CloneCommand::execute()
    {
        creative_kernel::setVisOperationMode(nullptr);
        sensorAnlyticsTrace("Model Editing & Layout", "Clone");
    }

    bool CloneCommand::checkSelect()
    {
        QList<ModelN*> selections = selectionms();
        if (selections.size() > 0)
        {
            return true;
        }
        return false;
    }
}
