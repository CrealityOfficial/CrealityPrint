#include "usecoursecommand.h"
#include "interface/commandinterface.h"

namespace creative_kernel
{
    UseCourseCommand::UseCourseCommand(QObject* parent)
        : ActionCommand(parent)
    {
        m_actionname = tr("Use Course");
        m_actionNameWithout = "Use Course";
        m_eParentMenu = eMenuType_Help;

        addUIVisualTracer(this,this);
    }

    UseCourseCommand::~UseCourseCommand()
    {
    }

    void UseCourseCommand::execute()
    {
       openUseTutorialWebsite();
    }

    void UseCourseCommand::onThemeChanged(ThemeCategory category)
    {

    }

    void UseCourseCommand::onLanguageChanged(MultiLanguage language)
    {
        m_actionname = tr("Use Course");
    }
}

