#include "pickmode.h"
#include "interface/visualsceneinterface.h"
#include "interface/commandinterface.h"

namespace creative_kernel
{
    PickMode::PickMode(QObject* parent)
        : ToolCommand(parent)
        , m_pickOp(nullptr)
    {
        orderindex = 0;
        m_name = tr("Pick") + "M";

        addUIVisualTracer(this);
        m_source = "qrc:/CrealityUI/qml/MovePanel.qml";
    }

    PickMode::~PickMode()
    {

    }

    void PickMode::onThemeChanged(ThemeCategory category)
    {
        setDisabledIcon(category == ThemeCategory::tc_dark ? "qrc:/UI/photo/leftBar/pick_dark.svg" : "qrc:/UI/photo/leftBar/pick_lite.svg");
        setEnabledIcon(category == ThemeCategory::tc_dark ? "qrc:/UI/photo/leftBar/pick_dark.svg" : "qrc:/UI/photo/leftBar/pick_lite.svg");
        setHoveredIcon(category == ThemeCategory::tc_dark ? "qrc:/UI/photo/leftBar/pick_pressed.svg" : "qrc:/UI/photo/leftBar/pick_lite.svg");
        setPressedIcon(category == ThemeCategory::tc_dark ? "qrc:/UI/photo/leftBar/pick_pressed.svg" : "qrc:/UI/photo/leftBar/pick_pressed.svg");
    }

    void PickMode::onLanguageChanged(MultiLanguage language)
    {
        m_name = tr("Pick") + " :M";
    }

    void PickMode::execute()
    {
        if (!m_pickOp)
        {
            m_pickOp = new PickOp(this);
        }
        setVisOperationMode(m_pickOp);
    }
}
