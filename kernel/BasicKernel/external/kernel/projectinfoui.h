#ifndef CREATIVE_KERNEL_PROJECTINFOUI_1594457868780_H
#define CREATIVE_KERNEL_PROJECTINFOUI_1594457868780_H
#include "basickernelexport.h"
#include "data/interface.h"
#include <QTimer>

namespace creative_kernel
{
    class ProjectOpenCallback
    {
    public:
        virtual ~ProjectOpenCallback() {}

        virtual void accept() = 0;
        virtual void cancel() = 0;
    };

    class BASIC_KERNEL_API ProjectInfoUI : public QObject
        , public UIVisualTracer
    {
        Q_OBJECT
    public:
        ProjectInfoUI(QObject* parent = nullptr);
        virtual ~ProjectInfoUI();

        static ProjectInfoUI* instance();
        static ProjectInfoUI* createInstance(QObject* parent);
        float getMinutes();
        void setMinute(float fMinute);


        QString getProjectName();
        void setProjectName(QString strProName);

        QString getProjectPath();
        void setProjectPath(QString strProPath);
        void deleteTempProjectDirectory();

        void setSettingsData(QString file);
        QString getAutoProjectPath();

        void updateProjectNameUI();
        QString getNameFromPath(QString path);

        void clearSecen();
        void requestMenuDialog(ProjectOpenCallback* callback);

        void updateFileStateUI();
        
        Q_INVOKABLE QString getMessageText();
        Q_INVOKABLE void accept();
        Q_INVOKABLE void cancel();
    signals:
        void minutesChanged(float fMinute);
    protected:
        void onThemeChanged(ThemeCategory category) override;
        void onLanguageChanged(MultiLanguage language) override;
    private:
        float m_fMinutes = 0.0;
        QString m_strProjectPath;
        QString m_strProjectName;
        QString m_strMessageText;
        QString m_strTempFilePath;

        static ProjectInfoUI* m_info;

        ProjectOpenCallback* m_callback;
    };
}

#endif // CREATIVE_KERNEL_PROJECTINFOUI_1594457868780_H
