#ifndef _LAYOUTCOMMAND_1592892614853_H
#define _LAYOUTCOMMAND_1592892614853_H
#include "menu/actioncommand.h"
#include "data/interface.h"

namespace creative_kernel
{
	class LayoutCommand : public ActionCommand
		, public UIVisualTracer
	{
		Q_OBJECT
	public:
		LayoutCommand(QObject* parent = nullptr);
		virtual ~LayoutCommand();

		Q_INVOKABLE void execute();
		bool enabled() override;

	protected:
		void onThemeChanged(ThemeCategory category) override;
		void onLanguageChanged(MultiLanguage language) override;
	};
}
#endif // _LAYOUTCOMMAND_1592892614853_H