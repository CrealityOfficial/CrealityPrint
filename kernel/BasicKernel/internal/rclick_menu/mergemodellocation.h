#ifndef MERGEMODELLOCATION_H
#define MERGEMODELLOCATION_H
#include "menu/actioncommand.h"
#include "data/interface.h"

namespace creative_kernel
{
    class MergeModelLocation :public ActionCommand
        , public UIVisualTracer
    {
        Q_OBJECT
    public:
        MergeModelLocation(QObject* parent = nullptr);
        virtual ~MergeModelLocation();

        Q_INVOKABLE void execute();
        bool enabled() override;
    protected:
        void onThemeChanged(ThemeCategory category) override;
        void onLanguageChanged(MultiLanguage language) override;
    };
}

#endif // MERGEMODELLOCATION_H
