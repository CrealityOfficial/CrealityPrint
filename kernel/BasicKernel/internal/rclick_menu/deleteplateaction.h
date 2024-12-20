#ifndef DELETE_PLATE_ACTION_H
#define DELETE_PLATE_ACTION_H
#include "menu/actioncommand.h"
#include "data/interface.h"

namespace creative_kernel
{
    class DeletePlateAction : public ActionCommand
        , public UIVisualTracer
    {
        Q_OBJECT
    public:
        DeletePlateAction(QObject* parent = nullptr);
        virtual ~DeletePlateAction();

        Q_INVOKABLE void execute();
        bool enabled() override;
    protected:
        void onThemeChanged(ThemeCategory category) override;
        void onLanguageChanged(MultiLanguage language) override;

    };
}

#endif // DELETE_PLATE_ACTION_H
