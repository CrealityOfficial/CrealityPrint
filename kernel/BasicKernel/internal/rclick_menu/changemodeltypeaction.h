#ifndef CHANGEMODELTYPEACTION_H
#define CHANGEMODELTYPEACTION_H
#include "menu/actioncommand.h"
#include "data/interface.h"

namespace creative_kernel
{
    class ChangeModelTypeAction :public ActionCommand
        , public UIVisualTracer
    {
        Q_OBJECT
    public:
        ChangeModelTypeAction(QObject* parent = nullptr);
        virtual ~ChangeModelTypeAction();

        Q_INVOKABLE void setModelType(int type);
        Q_INVOKABLE void execute();
        bool enabled() override;
    protected:
        void onThemeChanged(ThemeCategory category) override;
        void onLanguageChanged(MultiLanguage language) override;

    private:
        int m_type {0};

    };
}

#endif // CHANGEMODELTYPEACTION_H
