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
        Q_PROPERTY(bool projectDirty READ spaceDirty NOTIFY spaceDirtyChanged)
    public:
        ProjectInfoUI(QObject* parent = nullptr);
        virtual ~ProjectInfoUI();

        static ProjectInfoUI* instance();
        static ProjectInfoUI* createInstance(QObject* parent);
        Q_INVOKABLE void setSpaceDirty(bool _spaceDirty);
        bool spaceDirty();


        Q_INVOKABLE QString getProjectNameNoSuffix();
        bool is3mf();

        QString getProjectName();
        void setProjectName(QString strProName);

        QString getProjectPath();
        void setProjectPath(QString strProPath);
        Q_INVOKABLE void deleteTempProjectDirectory();

        void setSettingsData(QString file);
        QString getAutoProjectPath();

        void updateProjectNameUI();
        QString getNameFromPath(QString path);

        Q_INVOKABLE void clearSecen(bool clearModel = true);
        void requestMenuDialog(ProjectOpenCallback* callback);

        void updateFileStateUI();
        
        Q_INVOKABLE QString getMessageText();
        Q_INVOKABLE void accept();
        Q_INVOKABLE void cancel();

        bool availablePath();
    protected:
        void onThemeChanged(ThemeCategory category) override;
        void onLanguageChanged(MultiLanguage language) override;

    signals:
        void projectNameChanged();
        void spaceDirtyChanged();
    private:
        QString m_strProjectPath;
        QString m_strProjectName;
        QString m_strMessageText;
        QString m_strTempFilePath;

        QString m_strTmpCacheProjectPath;

        static ProjectInfoUI* m_info;

        ProjectOpenCallback* m_callback;
        bool m_spaceDirty;
    };
}

#endif // CREATIVE_KERNEL_PROJECTINFOUI_1594457868780_H
