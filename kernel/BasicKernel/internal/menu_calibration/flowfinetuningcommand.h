#ifndef FLOWFINETUNING_H
#define FLOWFINETUNING_H
#include "menu/actioncommand.h"
#include "data/interface.h"

namespace creative_kernel
{
	class FlowFineTuningCommand : public ActionCommand
		, public UIVisualTracer
	{
		Q_OBJECT
	public:
		FlowFineTuningCommand(QObject* parent = nullptr);
		virtual ~FlowFineTuningCommand();
		Q_INVOKABLE void execute();
		Q_INVOKABLE void getTuningTutorial();
		Q_INVOKABLE QString getWebsite();
		Q_INVOKABLE QString getCurrentVersion();
	protected:
		void onThemeChanged(ThemeCategory category) override;
		void onLanguageChanged(MultiLanguage language) override;
	};
}
#endif // !FLOWFINETUNING_H
