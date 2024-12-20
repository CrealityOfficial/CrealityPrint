#ifndef RESETALLACTION_H
#define RESETALLACTION_H
#include "menu/actioncommand.h"
#include "data/interface.h"

namespace creative_kernel
{
    class ResetAllAction : public ActionCommand
        , public UIVisualTracer
    {
        Q_OBJECT
    public:
        ResetAllAction(QObject* parent = nullptr);
        virtual ~ResetAllAction();

        Q_INVOKABLE void execute();
        Q_INVOKABLE QString getMessageText();
        void requestMenuDialog();

        Q_INVOKABLE void accept();
        bool enabled() override;
    protected:
        void onThemeChanged(ThemeCategory category) override;
        void onLanguageChanged(MultiLanguage language) override;
    private:
        QString m_strMessageText;
    };
}

#endif // RESETALLACTION_H
