#ifndef _SAVEASCOMMAND_H
#define _SAVEASCOMMAND_H
#include "menu/actioncommand.h"
#include "qtusercore/module/cxhandlerbase.h"
#include "data/interface.h"

namespace creative_kernel
{
	class SaveAsCommand : public ActionCommand
		, public UIVisualTracer
		, public qtuser_core::CXHandleBase 
	{
		Q_OBJECT
	public:
		SaveAsCommand(QObject* parent = nullptr);
		virtual ~SaveAsCommand();

		Q_INVOKABLE void execute();
		Q_INVOKABLE QString getMessageText();
		Q_INVOKABLE void accept();
	private:
		QString m_strMessageText;
	protected:
		void onThemeChanged(ThemeCategory category) override;
		void onLanguageChanged(MultiLanguage language) override;
		QString filter() override;
		void handle(const QString& fileName) override;
	protected:
		QString m_fileName;
	};
}

#endif // _SAVEASCOMMAND_H
