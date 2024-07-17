#include "logviewcommand.h"
#include "kernel/translator.h"
#include "interface/commandinterface.h"

namespace creative_kernel
{
	LogViewCommand::LogViewCommand(QObject* parent)
		:ActionCommand(parent)
	{
		m_actionname = tr("Log View");
		m_actionNameWithout = "Log View";
		m_eParentMenu = eMenuType_Tool;

		addUIVisualTracer(this,this);
	}

	LogViewCommand::~LogViewCommand()
	{

	}

	void LogViewCommand::execute()
	{
		openLogLocation();
	}

	void LogViewCommand::onThemeChanged(ThemeCategory category)
	{

	}

	void LogViewCommand::onLanguageChanged(MultiLanguage language)
	{
		m_actionname = tr("Log View");
	}
}
