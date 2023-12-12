#include "translator.h"
#include <QtCore/QTranslator>

#include <QtCore/QDebug>
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlApplicationEngine>
#include <QtCore/QSettings>
#include <QtWidgets/QApplication>

#include "data/interface.h"
#include "interface/appsettinginterface.h"

namespace creative_kernel
{
    Translator::Translator(QObject* parent)
        : QObject(parent)
        , m_engine(nullptr)
        , m_language(MultiLanguage::eLanguage_EN_TS)
        , m_theme(creative_kernel::ThemeCategory::tc_dark)
    {
        m_translator = new QTranslator(this);
        auto lang = QLocale().language();
        if (QLocale::Chinese == lang)
        {
            m_language = MultiLanguage::eLanguage_ZHCN_TS;
        }
        else if(QLocale::LiteraryChinese == lang)
        {
            m_language = MultiLanguage::eLanguage_ZHTW_TS;
        }
    }

    Translator::~Translator()
    {
    }

    MultiLanguage Translator::currentLanguage()
    {
        return m_language;
    }

    void Translator::addUIVisualTracer(creative_kernel::UIVisualTracer* tracer)
    {
        if (tracer && !m_uiVisualTracers.contains(tracer))
        {
            tracer->onLanguageChanged(currentLanguage());
            tracer->onThemeChanged(currentTheme());
            m_uiVisualTracers.append(tracer);
        }
    }

    void Translator::removeUIVisualTracer(creative_kernel::UIVisualTracer* tracer)
    {
        if (tracer && m_uiVisualTracers.contains(tracer))
            m_uiVisualTracers.removeOne(tracer);
    }

    void Translator::changeTheme(creative_kernel::ThemeCategory theme, bool force)
    {
        if (theme == m_theme && !force)
        {
            qDebug() << "changeTheme with same ThemeCategory.";
            return;
        }

        m_theme = theme;
        QSettings setting;
        setting.beginGroup("themecolor_config");
        setting.setValue("themecolor_config", (int)m_theme);
        setting.endGroup();

        for (creative_kernel::UIVisualTracer* tracer : m_uiVisualTracers)
            tracer->onThemeChanged(m_theme);
    }

    creative_kernel::ThemeCategory Translator::currentTheme()
    {
        return m_theme;
    }

    void Translator::loadUserSettings()
    {
        {
            QSettings setting;
            setting.beginGroup("themecolor_config");
            int nThemeType = setting.value("themecolor_config", 0).toInt();
            setting.endGroup();

            changeTheme((creative_kernel::ThemeCategory)nThemeType, true);
        }

        {
            QSettings setting;
            setting.beginGroup("language_perfer_config");
            QString lanTsFile = setting.value("language_type", "").toString();
            if (lanTsFile == "")
            {
                QString sysLang = QLocale::system().name();
                if (sysLang.indexOf("en") != -1)
                {
                    sysLang = "en";
                }
                if (sysLang == "zh_HK")
                {
                    sysLang = "zh_TW";
                }

                if (sysLang != "en" && sysLang != "zh_CN" && sysLang != "zh_TW")
                {
                    sysLang = "en";
                }

                lanTsFile = sysLang + ".ts";
            }
            changeLanguage(tsFile2Language(lanTsFile), true);
        }
    }

    void Translator::setQmlEngine(QQmlEngine* engine)
    {
        m_engine = engine;
    }

    void Translator::loadLanguage_ts(QString strFileName)
    {
        if (m_translator->load(strFileName))
        {
            QApplication::installTranslator(m_translator);
        }
    }
    
    void Translator::changeLanguage(MultiLanguage language, bool force)
    {
        if (language == m_language && !force)
        {
            qDebug() << "changeTheme with same ThemeCategory.";
            return;
        }

        m_language = language;
        QString tsFile = languageTsFile(m_language);

        QSettings setting;
        setting.beginGroup("language_perfer_config");
        setting.setValue("language_type", tsFile);
        setting.endGroup();

        QString strPath = ":/translations/" + tsFile;
        loadLanguage_ts(strPath);

        for (creative_kernel::UIVisualTracer* tracer : m_uiVisualTracers)
        {
            tracer->onLanguageChanged(m_language);
            //QObject* p = dynamic_cast<QObject*>(tracer);
            //QObject* p1 = p->parent();
            //if (p1 == nullptr)
            //{
                
            //}
        }
            

        if (m_engine)
            m_engine->retranslate();
    }
}
