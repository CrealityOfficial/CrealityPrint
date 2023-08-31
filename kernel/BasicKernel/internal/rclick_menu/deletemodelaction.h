#ifndef DELETEMODELACTION_H
#define DELETEMODELACTION_H
#include "menu/actioncommand.h"
#include "data/interface.h"

namespace creative_kernel
{
    class DeleteModelAction :public ActionCommand
        , public UIVisualTracer
    {
        Q_OBJECT
    public:
        DeleteModelAction(QObject* parent = nullptr);
        virtual ~DeleteModelAction();

        Q_INVOKABLE void execute();
        Q_INVOKABLE QString getMessageText();
        Q_INVOKABLE void writeReg(bool flag);
        Q_INVOKABLE bool isPopPage();
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

#endif // DELETEMODELACTION_H
