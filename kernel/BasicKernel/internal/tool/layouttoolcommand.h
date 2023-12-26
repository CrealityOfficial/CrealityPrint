#ifndef _NULLSPACE_LAYOUTTOOLCOMMAND_1589427153657_H
#define _NULLSPACE_LAYOUTTOOLCOMMAND_1589427153657_H
#include "qtusercore/plugin/toolcommand.h"
#include "operation/pickop.h"
#include "data/interface.h"

namespace creative_kernel
{
    class LayoutToolCommand : public ToolCommand, public UIVisualTracer
    {
        Q_OBJECT

    public:
        LayoutToolCommand(QObject* parent = nullptr);
        virtual ~LayoutToolCommand();
        Q_INVOKABLE void execute();
		Q_INVOKABLE bool isSelect();
        Q_INVOKABLE bool checkSelect() override;
		Q_INVOKABLE void layoutByType(int layoutType) const;

    protected:
        void onThemeChanged(ThemeCategory category) override;
        void onLanguageChanged(MultiLanguage language) override;

	private:
        PickOp* m_pickOp = nullptr;
    };

    void layoutByType(int type);
}
#endif // _NULLSPACE_LAYOUTTOOLCOMMAND_1589427153657_H
