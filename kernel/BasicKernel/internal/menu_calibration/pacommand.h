#ifndef PACOMMAND_H
#define PACOMMAND_H
#include "menu/actioncommand.h"
#include "data/interface.h"

namespace creative_kernel
{
	class PACommand : public ActionCommand
		, public UIVisualTracer
	{
		Q_OBJECT
	public:
		PACommand(QObject* parent = nullptr);
		virtual ~PACommand();

		Q_INVOKABLE void execute();
		Q_INVOKABLE QString getWebsite();
		Q_INVOKABLE QString getCurrentVersion();
	protected:
		void onThemeChanged(ThemeCategory category) override;
		void onLanguageChanged(MultiLanguage language) override;
	};
}
#endif // !PACOMMAND_H
