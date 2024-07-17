#ifndef SETNOZZLERANGE_H
#define SETNOZZLERANGE_H
#include "menu/actioncommand.h"
#include "menu/actioncommandmodel.h"
#include "data/interface.h"

namespace creative_kernel
{
    class SetNozzleRange : public ActionCommand
        , public UIVisualTracer
    {
        Q_OBJECT
        Q_PROPERTY(QAbstractListModel* subMenuActionmodel READ subMenuActionmodel CONSTANT)
    public:
        SetNozzleRange(QObject* parent = nullptr);
        virtual ~SetNozzleRange();

        QAbstractListModel* subMenuActionmodel();
        Q_INVOKABLE void setNozzleCount(int nCount);
        Q_INVOKABLE void setExtruderColors(QList<QColor> colors);
        void updateNozzleCheckStatus(int index, bool checked);
        bool enabled() override;
    protected:
        void updateActionModel();
        void onThemeChanged(ThemeCategory category) override;
        void onLanguageChanged(MultiLanguage language) override;

    protected:
        int m_nNozzleCount=0;
        QList<QColor> m_ExtruderColors;
        ActionCommandModel* m_actionModelList = nullptr;
    };
}
#endif // SETNOZZLERANGE_H
