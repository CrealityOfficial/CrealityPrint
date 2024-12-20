#ifndef SUBMENUSTANDARDPART_H
#define SUBMENUSTANDARDPART_H
#include "menu/actioncommand.h"
#include "menu/actioncommandmodel.h"
#include "data/interface.h"
#include <QtCore/QSettings>
#include "data/kernelenum.h"

namespace creative_kernel
{
    class BASIC_KERNEL_API SubMenuStandardPart : public ActionCommand
        , public UIVisualTracer
    {
        Q_OBJECT
        Q_PROPERTY(QAbstractListModel* subMenuActionmodel READ subMenuActionmodel CONSTANT)
    public:
        explicit SubMenuStandardPart(ModelNType partType, QObject* parent = NULL);
        virtual ~SubMenuStandardPart();

        void initActionModel();
        QAbstractListModel* subMenuActionmodel();

    protected:
        void onThemeChanged(ThemeCategory category) override;
        void onLanguageChanged(MultiLanguage language) override;

    private:
        ActionCommandModel* m_actionModelList = nullptr;
        ModelNType m_partType { ModelNType::normal_part };
    };
}

#endif // SUBMENUSTANDARDPART_H
