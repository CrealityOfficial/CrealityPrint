#include "modelshowcommand.h"
#include "interface/modelinterface.h"
#include "interface/commandinterface.h"

namespace creative_kernel
{
    ModelShowCommand::ModelShowCommand(ModelVisualMode mode, QObject* parent)
        : ActionCommand(parent)
        , m_mode(mode)
    {
        m_eParentMenu = eMenuType_View;
        addUIVisualTracer(this);
    }

    ModelShowCommand::~ModelShowCommand()
    {
    }

    void ModelShowCommand::execute()
    {
        setModelVisualMode(m_mode);
    }

    void ModelShowCommand::onThemeChanged(ThemeCategory category)
    {

    }

    void ModelShowCommand::onLanguageChanged(MultiLanguage language)
    {
        switch (m_mode)
        {
        case ModelVisualMode::mvm_line:
            m_actionname = tr("ShowModelLine");
            m_actionNameWithout = "ShowModelLine";
            break;
        case ModelVisualMode::mvm_face:
            m_actionname = tr("ShowModelFace");
            m_actionNameWithout = "ShowModelFace";
            break;
        case ModelVisualMode::mvm_line_and_face:
            m_actionname = tr("ShowModelFaceLine");
            m_actionNameWithout = "ShowModelFaceLine";
            break;
        }
    }
}
