#ifndef CLEARALLACTION_H
#define CLEARALLACTION_H
#include "menu/actioncommand.h"
#include "data/interface.h"

namespace creative_kernel
{
    class ClearAllAction : public ActionCommand
        , public UIVisualTracer
    {
        Q_OBJECT
    public:
        ClearAllAction(QObject* parent = nullptr);
        virtual ~ClearAllAction();

        Q_INVOKABLE void execute();
        Q_INVOKABLE QString getMessageText();

        Q_INVOKABLE void accept();
        bool enabled() override;
    protected:
        void onThemeChanged(ThemeCategory category) override;
        void onLanguageChanged(MultiLanguage language) override;
    private:
        QString m_strMessageText;
    };
}

#endif // CLEARALLACTION_H
