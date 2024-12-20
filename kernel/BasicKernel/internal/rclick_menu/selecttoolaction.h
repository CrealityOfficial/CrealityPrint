#ifndef SELECT_TOOL_ACTION_H
#define SELECT_TOOL_ACTION_H
#include "menu/actioncommand.h"
#include "data/interface.h"

namespace creative_kernel
{
    class SelectToolAction : public ActionCommand
        , public UIVisualTracer
    {
        Q_OBJECT
    public:
        SelectToolAction(const QString& toolName, int toolId, QObject* parent = nullptr);
        virtual ~SelectToolAction();

        Q_INVOKABLE void execute();
        bool enabled() override;
    protected:
        void onThemeChanged(ThemeCategory category) override;
        void onLanguageChanged(MultiLanguage language) override;

    private:
        int m_toolId;

    };
}

#endif // SELECT_TOOL_ACTION_H
