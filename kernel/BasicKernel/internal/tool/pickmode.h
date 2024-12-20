#ifndef _NULLSPACE_PICKMODE_1589765331924_H
#define _NULLSPACE_PICKMODE_1589765331924_H
#include "qtusercore/plugin/toolcommand.h"
#include "operation/pickop.h"
#include "data/interface.h"

namespace creative_kernel
{
	class PickMode : public ToolCommand
		, public UIVisualTracer
	{
		Q_OBJECT
	public:
		PickMode(QObject* parent = nullptr);
		virtual ~PickMode();

		Q_INVOKABLE void execute();
	protected:
		void onThemeChanged(ThemeCategory category) override;
		void onLanguageChanged(MultiLanguage language) override;
	protected:
		PickOp* m_pickOp;
	};
}
#endif // _NULLSPACE_PICKMODE_1589765331924_H
