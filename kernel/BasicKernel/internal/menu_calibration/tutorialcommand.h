#ifndef TUTORIALCOMMAND_H
#define TUTORIALCOMMAND_H
#include "menu/actioncommand.h"
#include "data/interface.h"

namespace creative_kernel
{
	class TutorialCommand : public ActionCommand
		, public UIVisualTracer
	{
		Q_OBJECT
	public:
		TutorialCommand(QObject* parent = nullptr);
		virtual ~TutorialCommand();

		Q_INVOKABLE void execute();
		Q_INVOKABLE QString getWebsite();
		Q_INVOKABLE QString getCurrentVersion();
	protected:
		void onThemeChanged(ThemeCategory category) override;
		void onLanguageChanged(MultiLanguage language) override;
	};
}
#endif // !TUTORIALCOMMAND_H
