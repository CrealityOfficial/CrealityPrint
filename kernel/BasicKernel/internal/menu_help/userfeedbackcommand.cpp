#include "userfeedbackcommand.h"
#include "kernel/translator.h"

#include "interface/commandinterface.h"

namespace creative_kernel
{
	UserFeedbackCommand::UserFeedbackCommand(QObject* parent)
		: ActionCommand(parent)
	{
		m_actionname = tr("User Feedback");
		m_actionNameWithout = "User Feedback";
		m_eParentMenu = eMenuType_Help;

		addUIVisualTracer(this,this);
	}

	UserFeedbackCommand::~UserFeedbackCommand()
	{
	}

	void UserFeedbackCommand::execute()
	{
		openUserFeedbackWebsite();
	}

	void UserFeedbackCommand::onThemeChanged(ThemeCategory category)
	{

	}

	void UserFeedbackCommand::onLanguageChanged(MultiLanguage language)
	{
		m_actionname = tr("User Feedback");
	}
}
