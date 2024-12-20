#include "modeleffectcommand.h"
#include "interface/selectorinterface.h"
#include "interface/visualsceneinterface.h"

#include "kernel/kernelui.h"
#include "interface/commandinterface.h"
#include "interface/uiinterface.h"
#include "modeleffectop.h"

using namespace creative_kernel;

ModelEffectCommand::ModelEffectCommand(QObject* parent) 
    : ToolCommand(parent)
    , m_op(nullptr)
{
    addUIVisualTracer(this);
}

ModelEffectCommand::~ModelEffectCommand()
{

}

//void ModelEffectCommand::onThemeChanged(creative_kernel::ThemeCategory category)
//{
//}
//
//void ModelEffectCommand::onLanguageChanged(creative_kernel::MultiLanguage language)
//{
//}

bool ModelEffectCommand::message()
{
    return true;
}

void ModelEffectCommand::setMessage(bool isRemove)
{
}

void ModelEffectCommand::execute()
{
    if (!m_op)
    {
        m_op = new ModelEffectOp(this);
    }

    if (visOperationMode() != m_op)
    {
        setVisOperationMode(m_op);
    }
    else
    {
        setVisOperationMode(nullptr);
    }
}

bool ModelEffectCommand::checkSelect()
{
    QList<ModelN*> selections = selectionms();
    if(selections.size()>0)
    {
        return true;
    }
    return false;
}

void ModelEffectCommand::accept()
{
    setMessage(true);
}

void ModelEffectCommand::cancel()
{
    setMessage(false);
}

