#ifndef FLOWCOARSETUNING_H
#define FLOWCOARSETUNING_H
#include "menu/actioncommand.h"
#include "data/interface.h"

namespace creative_kernel
{
	class FlowCoarseTuningCommand : public ActionCommand
		, public UIVisualTracer
	{
		Q_OBJECT
	public:
		FlowCoarseTuningCommand(QObject* parent = nullptr);
		virtual ~FlowCoarseTuningCommand();

		Q_INVOKABLE void execute();
		Q_INVOKABLE QString getWebsite();
		Q_INVOKABLE QString getCurrentVersion();
	protected:
		void onThemeChanged(ThemeCategory category) override;
		void onLanguageChanged(MultiLanguage language) override;
	};
}
#endif // !PACOMMAND_H
