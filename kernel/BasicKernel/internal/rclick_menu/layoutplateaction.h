#ifndef LAYOUT_PLATE_ACTION_H
#define LAYOUT_PLATE_ACTION_H
#include "menu/actioncommand.h"
#include "data/interface.h"

namespace creative_kernel
{
	class LayoutPlateAction : public ActionCommand
		, public UIVisualTracer
	{
		Q_OBJECT
	public:
		LayoutPlateAction(QObject* parent = nullptr);
		virtual ~LayoutPlateAction();

		Q_INVOKABLE void execute();
		bool enabled() override;

	protected:
		void onThemeChanged(ThemeCategory category) override;
		void onLanguageChanged(MultiLanguage language) override;
	};
}
#endif // LAYOUT_PLATE_ACTION_H