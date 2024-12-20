#ifndef _RESETACTIONCOMMAND_H
#define _RESETACTIONCOMMAND_H
#include "menu/actioncommand.h"
#include "qtusercore/plugin/toolcommand.h"
#include "data/interface.h"

namespace creative_kernel
{
	class ResetActionCommand : public ActionCommand
		, public UIVisualTracer
	{
		Q_OBJECT
	public:
		ResetActionCommand(QObject* parent = nullptr);
		virtual ~ResetActionCommand();
		Q_INVOKABLE void execute();
	protected:
		void onThemeChanged(ThemeCategory category) override;
		void onLanguageChanged(MultiLanguage language) override;
	};
}
#endif // _RESETACTIONCOMMAND_H
