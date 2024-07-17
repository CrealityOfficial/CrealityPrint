#ifndef _MODELSHOWCOMMAND_1595475565671_H
#define _MODELSHOWCOMMAND_1595475565671_H
#include "menu/actioncommand.h"
#include "data/kernelenum.h"
#include "data/interface.h"

namespace creative_kernel
{
    class ModelShowCommand : public ActionCommand
        , public UIVisualTracer
    {
        Q_OBJECT
    public:
        ModelShowCommand(ModelVisualMode mode, QObject* parent = nullptr);
        virtual ~ModelShowCommand();
        bool enabled();
        Q_INVOKABLE void updateCheck();
        Q_INVOKABLE void execute();
    protected:
        void onThemeChanged(ThemeCategory category) override;
        void onLanguageChanged(MultiLanguage language) override;
    protected:
        ModelVisualMode m_mode = ModelVisualMode::mvm_face;
    };
}
#endif // _MODELSHOWCOMMAND_1595475565671_H
