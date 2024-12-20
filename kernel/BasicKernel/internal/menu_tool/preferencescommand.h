#ifndef PREFERENCESCOMMAND_H
#define PREFERENCESCOMMAND_H
#include "menu/actioncommand.h"
#include "data/interface.h"

namespace creative_kernel
{
	class PreferencesCommand : public ActionCommand
		, public UIVisualTracer
	{
		Q_OBJECT
	public:
		PreferencesCommand(QObject* parent = nullptr);
		virtual ~PreferencesCommand();

		Q_INVOKABLE void execute();
		Q_INVOKABLE void generate(int _statrt,int _end,int _step);
	protected:
		void onThemeChanged(ThemeCategory category) override;
		void onLanguageChanged(MultiLanguage language) override;

	private:
	};
}
#endif // !PREFERENCESCOMMAND_H
