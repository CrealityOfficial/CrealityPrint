#ifndef SPLITMODELACTION_H
#define SPLITMODELACTION_H
#include "menu/actioncommand.h"
#include "data/interface.h"

namespace creative_kernel
{
    class SplitModelAction : public ActionCommand
        , public UIVisualTracer
    {
        Q_OBJECT
    public:
        SplitModelAction(QObject* parent = nullptr);
        virtual ~SplitModelAction();

        Q_INVOKABLE void execute();
        bool enabled() override;
    protected:
        void onThemeChanged(ThemeCategory category) override;
        void onLanguageChanged(MultiLanguage language) override;
    private:
        QString m_strFilePath;
    };
}
#endif // SPLITMODELACTION_H
