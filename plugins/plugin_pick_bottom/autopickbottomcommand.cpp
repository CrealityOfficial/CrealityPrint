#include "autopickbottomcommand.h"
#include "pickbottomop.h"
#include "interface/selectorinterface.h"
#include "interface/visualsceneinterface.h"
#include "kernel/translator.h"

#include "kernel/kernelui.h"
#include "interface/commandinterface.h"
#include "interface/uiinterface.h"
#include "interface/spaceinterface.h"
#include "interface/modelinterface.h"
using namespace creative_kernel;

AutoPickBottomCommand::AutoPickBottomCommand(QObject* parent) : ToolCommand(parent), m_op(nullptr)
{
    m_name = tr("P Bottom") + ": F";
    //setSource("qrc:/pickbottom/pickbottom/pickBottom.qml");

    addUIVisualTracer(this,this);
}

AutoPickBottomCommand::~AutoPickBottomCommand()
{

}

void AutoPickBottomCommand::onThemeChanged(creative_kernel::ThemeCategory category)
{
    setDisabledIcon(category == ThemeCategory::tc_dark ? "qrc:/UI/photo/leftBar/auto_pick_dark.svg" : "qrc:/UI/photo/leftBar/auto_pick_dark.svg");
    setEnabledIcon(category == ThemeCategory::tc_dark ? "qrc:/UI/photo/leftBar/auto_pick_dark_d.svg" : "qrc:/UI/photo/leftBar/auto_pick_dark_d.svg");
    setHoveredIcon(category == ThemeCategory::tc_dark ? "qrc:/UI/photo/leftBar/auto_pick_dark_d.svg" : "qrc:/UI/photo/leftBar/auto_pick_dark_d.svg");
    setPressedIcon(category == ThemeCategory::tc_dark ? "qrc:/UI/photo/leftBar/auto_pick_dark_d.svg" : "qrc:/UI/photo/leftBar/auto_pick_dark_d.svg");
}
bool AutoPickBottomCommand::isCFunction()
{
    return true;
    }
void AutoPickBottomCommand::onLanguageChanged(creative_kernel::MultiLanguage language)
{
    m_name = tr("Auto Orient") + ": A";
}


void AutoPickBottomCommand::execute()
{

    creative_kernel::setModelsMaxFaceBottom(modelns());
    getKernelUI()->setCommandIndex(-1);
    sensorAnlyticsTrace("Model Editing & Layout", "Place on Face");
}

bool AutoPickBottomCommand::checkSelect()
{
    QList<ModelN*> selections = selectionms();
    if(selections.size()>0)
    {
        return true;
    }
    return false;
}

void AutoPickBottomCommand::accept()
{
    
}

void AutoPickBottomCommand::cancel()
{
    getKernelUI()->switchPickMode();
}

 void AutoPickBottomCommand::maxFaceBottom()
{
    m_op->setMaxFaceBottom();
}
