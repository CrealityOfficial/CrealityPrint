#ifndef _MIRRORACTIONCOMMAND_H
#define _MIRRORACTIONCOMMAND_H
#include "menu/actioncommand.h"
#include "data/kernelenum.h"
#include "data/interface.h"

namespace creative_kernel
{
    class MirrorActionCommand : public ActionCommand
        , public UIVisualTracer
    {
        Q_OBJECT
    public:
        MirrorActionCommand(MirrorOperation operation, QObject* parent = nullptr);
        virtual ~MirrorActionCommand();

        Q_INVOKABLE void execute();
        void mirrorX();
        void mirrorY();
        void mirrorZ();
        void reset();
    protected:
        void onThemeChanged(ThemeCategory category) override;
        void onLanguageChanged(MultiLanguage language) override;

    protected:
        MirrorOperation m_operation = MirrorOperation::mo_reset;
    };
}
#endif // _MIRRORACTIONCOMMAND_H
