#ifndef _NULLSPACE_CLONECOMMAND_1589427153657_H
#define _NULLSPACE_CLONECOMMAND_1589427153657_H
#include "qtusercore/plugin/toolcommand.h"
#include "data/interface.h"

namespace creative_kernel
{
	class CloneCommand : public ToolCommand
		, public UIVisualTracer
	{
		Q_OBJECT
	public:
		CloneCommand(QObject* parent = nullptr);
		virtual ~CloneCommand();
		Q_INVOKABLE bool isSelect();
		Q_INVOKABLE void clone(int numb);
		Q_INVOKABLE void execute();
		Q_INVOKABLE bool checkSelect() override;
	protected:
		void onThemeChanged(ThemeCategory category) override;
		void onLanguageChanged(MultiLanguage language) override;
	};
}
#endif // _NULLSPACE_CLONECOMMAND_1589427153657_H
