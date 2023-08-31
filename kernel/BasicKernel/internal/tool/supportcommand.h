#ifndef _NULLSPACE_SUPPORTCOMMAND_1589427153657_H
#define _NULLSPACE_SUPPORTCOMMAND_1589427153657_H
#include "qtuserqml/plugin/toolcommand.h"
#include "operation/pickop.h"
#include "data/interface.h"

namespace creative_kernel
{
    class SupportCommand : public ToolCommand
        , public UIVisualTracer
    {
        Q_OBJECT
    public:
        SupportCommand(QObject* parent = nullptr);
        virtual ~SupportCommand();
        Q_INVOKABLE bool isSelect();
        Q_INVOKABLE void execute();
        Q_INVOKABLE bool checkSelect() override;
    protected:
        void onThemeChanged(ThemeCategory category) override;
        void onLanguageChanged(MultiLanguage language) override;
    protected:
        PickOp* m_pickOp = nullptr;
    };
}
#endif // _NULLSPACE_SUPPORTCOMMAND_1589427153657_H
