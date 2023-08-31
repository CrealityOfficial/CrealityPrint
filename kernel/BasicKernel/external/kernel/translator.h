#ifndef TRANSLATOR_H
#define TRANSLATOR_H
#include "basickernelexport.h"
#include "data/kernelenum.h"

class QTranslator;
class QQmlApplicationEngine;

namespace creative_kernel
{
    class UIVisualTracer;
    class BASIC_KERNEL_API Translator : public QObject
    {
        Q_OBJECT
    public:
        Translator(QObject* parent = 0);
        virtual ~Translator();

        void loadUserSettings();
        void setQmlEngine(QQmlApplicationEngine* engine);
        void changeLanguage(MultiLanguage language, bool force = false);
        MultiLanguage currentLanguage();

        void addUIVisualTracer(creative_kernel::UIVisualTracer* tracer);
        void removeUIVisualTracer(creative_kernel::UIVisualTracer* tracer);
        void changeTheme(creative_kernel::ThemeCategory theme, bool force = false);
        creative_kernel::ThemeCategory currentTheme();
    protected:
        void loadLanguage_ts(QString strFileName);

    private:
        QQmlApplicationEngine* m_engine;
        QTranslator* m_translator;

        MultiLanguage m_language;

        QList<creative_kernel::UIVisualTracer*> m_uiVisualTracers;
        creative_kernel::ThemeCategory m_theme;
    };
}

#endif // TRANSLATOR_H
