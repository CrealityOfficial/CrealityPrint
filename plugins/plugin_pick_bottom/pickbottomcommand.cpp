#include "pickbottomcommand.h"
#include "pickbottomop.h"
#include "interface/selectorinterface.h"
#include "interface/visualsceneinterface.h"
#include "kernel/translator.h"

#include "kernel/kernelui.h"
#include "interface/commandinterface.h"
#include "interface/uiinterface.h"
#include "interface/spaceinterface.h"
#include "interface/printerinterface.h"

using namespace creative_kernel;

PickBottomCommand::PickBottomCommand(QObject* parent) : ToolCommand(parent), m_op(nullptr)
{
    m_name = tr("P Bottom") + ": B";
    setSource("qrc:/pickbottom/pickbottom/pickBottom.qml");

    addUIVisualTracer(this,this);
}

PickBottomCommand::~PickBottomCommand()
{

}

void PickBottomCommand::onThemeChanged(creative_kernel::ThemeCategory category)
{
    setDisabledIcon("qrc:/UI/photo/cToolBar/faceFlat_dark_disable.svg" );
    setEnabledIcon(category == ThemeCategory::tc_dark ?"qrc:/UI/photo/cToolBar/faceFlat_dark_default.svg" : "qrc:/UI/photo/cToolBar/faceFlat_light_default.svg");
    setHoveredIcon(category == ThemeCategory::tc_dark ?"qrc:/UI/photo/cToolBar/faceFlat_dark_default.svg" : "qrc:/UI/photo/cToolBar/faceFlat_light_default.svg");
    setPressedIcon("qrc:/UI/photo/cToolBar/faceFlat_dark_press.svg");
}

void PickBottomCommand::onLanguageChanged(creative_kernel::MultiLanguage language)
{
    m_name = tr("P Bottom") + ": B";
}

void PickBottomCommand::slotMouseLeftClicked()
{
    message();
}

bool PickBottomCommand::message()
{
    if (m_op->getMessage())
    {
        requestQmlDialog(this, "deleteSupportDlg");
    }

    return true;
}

void PickBottomCommand::setMessage(bool isRemove)
{
    m_op->setMessage(isRemove);
}

void PickBottomCommand::execute()
{
	if (!m_op)
	{
		m_op = new PickBottomOp(this);
        connect(m_op, SIGNAL(mouseLeftClicked()), this, SLOT(slotMouseLeftClicked()));
	}

    if(visOperationMode() != m_op)
    {
        setVisOperationMode(m_op);
    }
    else
    {
        setVisOperationMode(nullptr);
    }

    sensorAnlyticsTrace("Model Editing & Layout", "Place on Face");
}

bool PickBottomCommand::checkSelect()
{
    QList<ModelN*> selections = selectionms();
    if(selections.size()>0)
    {
        return true;
    }
    return false;
}

void PickBottomCommand::accept()
{
    setMessage(true);
}

void PickBottomCommand::cancel()
{
    setMessage(false);
    getKernelUI()->switchPickMode();
}

void PickBottomCommand::autoPickBottom()
{
    QList<ModelGroup*> modelGroups = selectedGroups();
    if (modelGroups.empty())
    {
        QList<ModelGroup*> all = getAllPrinterModelsGroups();
        creative_kernel::setModelGroupsMaxFaceBottom(all);
    }
    else
    {
        creative_kernel::setModelGroupsMaxFaceBottom(modelGroups);
    }

}
