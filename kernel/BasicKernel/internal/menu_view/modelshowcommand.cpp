#include "modelshowcommand.h"
#include "interface/visualsceneinterface.h"
#include "interface/commandinterface.h"
#include <qtusercore/util/settings.h>

namespace creative_kernel
{
    ModelShowCommand::ModelShowCommand(ModelVisualMode mode, QObject* parent)
        : ActionCommand(parent)
        , m_mode(mode)
    {
        m_eParentMenu = eMenuType_View;
        qtuser_core::VersionSettings setting;
        setting.beginGroup("view_show");
        int nRender = setting.value("render_type").toInt();
        if (!nRender) {
            setting.setValue("render_type", (int)ModelVisualMode::mvm_face);
        }
        setting.endGroup();
        addUIVisualTracer(this,this);
    }

    ModelShowCommand::~ModelShowCommand()
    {
    }

    void ModelShowCommand::execute()
    {
        setModelVisualMode(m_mode);
        qtuser_core::VersionSettings setting;
        setting.beginGroup("view_show");
        setting.setValue("render_type", (int)m_mode);
        setting.endGroup();
    }
    bool ModelShowCommand::enabled() {
        qtuser_core::VersionSettings setting;
        setting.beginGroup("view_show");
        int nRender = setting.value("render_type").toInt();
        setting.endGroup();
        return nRender == (int)m_mode;
    }
    void ModelShowCommand::updateCheck() {
        update();
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
