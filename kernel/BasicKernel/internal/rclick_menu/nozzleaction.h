#ifndef NOZZLEACTION_H
#define NOZZLEACTION_H
#include "menu/actioncommand.h"
#include "data/interface.h"
#include <QColor>

namespace creative_kernel
{
    class PrintExtruder;
    class NozzleAction : public ActionCommand
        , public UIVisualTracer
    {
        Q_OBJECT
        Q_PROPERTY(QColor nozzleColor READ nozzleColor WRITE setNozzleColor NOTIFY nozzleColorChanged)

    public:
        NozzleAction(int nozzleIndex,QObject* parent, QString currentColor = QString());
        virtual ~NozzleAction();

        void setExtruderObj(PrintExtruder* extruderObj);

        QColor nozzleColor();
        void setNozzleColor(QColor color);

        Q_INVOKABLE void execute();
        bool enabled() override;

    signals:
        void nozzleColorChanged();

    protected:
        void onThemeChanged(ThemeCategory category) override;
        void onLanguageChanged(MultiLanguage language) override;

    private:
        int m_NozzleIndex = 1;
        QString m_CurrentColor;
        PrintExtruder* m_PrintExtruder = nullptr;
    };
}

#endif // NOZZLEACTION_H
