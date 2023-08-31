#include "supportcommand.h"

#include "data/modeln.h"

#include "interface/selectorinterface.h"
#include "interface/commandinterface.h"
#include "cxkernel/interface/jobsinterface.h"

#include "utils/modelpositioninitializer.h"

#include <QtCore/QDebug>
#include "interface/visualsceneinterface.h"
#include "kernel/translator.h"
#include "data/fdmsupportgroup.h"

#include "kernel/kernelui.h"
#include "interface/uiinterface.h"

namespace creative_kernel
{
    SupportCommand::SupportCommand(QObject* parent)
        :ToolCommand(parent)
    {
        orderindex = 5;
        m_name = tr("Support");
        m_source = "qrc:/CrealityUI/secondqml/CusSupportPanel.qml";
        addUIVisualTracer(this);
    }

    SupportCommand::~SupportCommand()
    {
    }

    void SupportCommand::onThemeChanged(ThemeCategory category)
    {
        setDisabledIcon(category == ThemeCategory::tc_dark ? "qrc:/UI/photo/leftBar/support_dark.svg" : "qrc:/UI/photo/leftBar/support_lite.svg");
        setEnabledIcon(category == ThemeCategory::tc_dark ? "qrc:/UI/photo/leftBar/support_dark.svg" : "qrc:/UI/photo/leftBar/support_lite.svg");
        setHoveredIcon(category == ThemeCategory::tc_dark ? "qrc:/UI/photo/leftBar/support_pressed.svg" : "qrc:/UI/photo/leftBar/support_lite.svg");
        setPressedIcon(category == ThemeCategory::tc_dark ? "qrc:/UI/photo/leftBar/support_pressed.svg" : "qrc:/UI/photo/leftBar/support_pressed.svg");
    }

    void SupportCommand::onLanguageChanged(MultiLanguage language)
    {
        m_name = tr("Support");
    }

    bool SupportCommand::isSelect()
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

    void SupportCommand::execute()
    {
        QObject* pmainWinbj = getKernelUI()->getUI("UIAppWindow");
        QMetaObject::invokeMethod(pmainWinbj, "supportTabBtnClicked");
        sensorAnlyticsTrace("Support", "Support (Button)");
    }

    bool SupportCommand::checkSelect()
    {
        QList<ModelN*> selections = selectionms();
        if (selections.size() > 0)
        {
            return true;
        }
        return false;
    }
}
