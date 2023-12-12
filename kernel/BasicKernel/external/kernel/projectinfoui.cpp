#include "projectinfoui.h"
#include <QtCore/QSettings>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtQml/QQmlProperty>

#include "qtusercore/string/resourcesfinder.h"
#include "kernel/kernelui.h"
#include "interface/modelinterface.h"
#include "interface/commandinterface.h"
#include "interface/uiinterface.h"

namespace creative_kernel
{
    ProjectInfoUI* ProjectInfoUI::m_info=nullptr;
    ProjectInfoUI::ProjectInfoUI(QObject* parent)
        : QObject(parent)
        , m_callback(nullptr)
    {
        QString tmpProject = qtuser_core::getOrCreateAppDataLocation("tmpProject");
        QDir tempDir;
        if (!tempDir.exists(tmpProject))
        {
            tempDir.mkpath(tmpProject);
        }
        m_strTempFilePath = qtuser_core::getOrCreateAppDataLocation("tmpProject") + "/default.cxprj";
        setSettingsData(m_strTempFilePath);

        QSettings setting;
        setting.beginGroup("AutoSave_Path");
        m_fMinutes = setting.value("saveminutes", "10.0").toFloat();
        setting.endGroup();
        //    m_fMinutes = 5.0;
        m_strProjectName = "";
        m_strProjectPath = "";
        m_strMessageText = tr("Do you Want to Save LastProject?");

        QString strFileName = setting.value("filePath", "default").toString();
        setting.endGroup();
        if (!strFileName.contains("default"))
        {
            m_strTempFilePath = strFileName;
        }

        addUIVisualTracer(this);
    }

    ProjectInfoUI::~ProjectInfoUI()
    {

    }
    ProjectInfoUI* ProjectInfoUI::instance()
    {
        //static ProjectInfoUI info;
        if (m_info == NULL)
            createInstance(NULL);
        return m_info;
    }
    ProjectInfoUI* ProjectInfoUI::createInstance(QObject* parent)
    {
        m_info = new ProjectInfoUI(parent);
        return m_info;
    }

    float ProjectInfoUI::getMinutes()
    {
        if (m_fMinutes == 0.0)
        {
            m_fMinutes = 10.0;
        }
        return m_fMinutes;
    }

    void ProjectInfoUI::setMinute(float fMinute)
    {
        if (m_fMinutes != fMinute)
        {
            emit minutesChanged(fMinute);
        }
        m_fMinutes = fMinute;
        QSettings setting;
        setting.beginGroup("AutoSave_Path");
        setting.setValue("saveminutes", m_fMinutes);
        setting.endGroup();
    }

    QString ProjectInfoUI::getProjectName()
    {
        return m_strProjectName;
    }

    void ProjectInfoUI::setProjectName(QString strProName)
    {
        m_strProjectName = strProName;
    }

    QString ProjectInfoUI::getProjectPath()
    {
        return m_strProjectPath;
    }

    void ProjectInfoUI::setProjectPath(QString strProPath)
    {
        m_strProjectPath = strProPath;
    }

    void ProjectInfoUI::deleteTempProjectDirectory()
    {
        QString pathDir = qtuser_core::getOrCreateAppDataLocation("") + "/tmpProject";
        QDir tempDir(pathDir);
        tempDir.removeRecursively();
    }

    QString ProjectInfoUI::getAutoProjectPath()
    {
        return m_strTempFilePath;
    }

    void ProjectInfoUI::setSettingsData(QString file)
    {
        file = file.replace("file:///", "");
        QSettings setting;
        setting.beginGroup("AutoSave_Path");
        setting.setValue("filePath", file);
        setting.endGroup();
    }

    void ProjectInfoUI::updateProjectNameUI()
    {
        QObject* obj = getKernelUI()->getUI("footer");    //   getKernelUI()->footer();
        QObject* pProText = obj->findChild<QObject*>("ProjectTitle");
        if (pProText)
        {
            QQmlProperty::write(pProText, "text", QVariant::fromValue(m_strProjectName));
        }
    }

    QString ProjectInfoUI::getNameFromPath(QString path)
    {
        QString strName = "";
        QFileInfo info(path);
        // qDebug()<<"info.fileName()=" << info.fileName();
        strName = info.fileName();
        return strName;
    }

    void ProjectInfoUI::clearSecen()
    {
        using namespace creative_kernel;
        removeAllModel(true);
    }

    void ProjectInfoUI::requestMenuDialog(ProjectOpenCallback* callback)
    {
        m_callback = callback;
        requestQmlDialog(this, "messageDlg");
    }

    QString ProjectInfoUI::getMessageText()
    {
        return m_strMessageText;
    }

    void ProjectInfoUI::onThemeChanged(ThemeCategory category)
    {

    }

    void ProjectInfoUI::onLanguageChanged(MultiLanguage language)
    {
        m_strMessageText = tr("Do you Want to Save LastProject?");
    }

    void ProjectInfoUI::accept()
    {
        if (m_callback)
            m_callback->accept();
        m_callback = nullptr;
    }
    
    void ProjectInfoUI::cancel()
    {
        if (m_callback)
            m_callback->cancel();
        m_callback = nullptr;
    }

    void ProjectInfoUI::updateFileStateUI()
    {
        QObject* obj = getKernelUI()->getUI("footer");    // getKernelUI()->footer();
        QObject* pProText = obj->findChild<QObject*>("SavefinishShow");
        if (pProText)
        {
            QQmlProperty::write(pProText, "visible", QVariant::fromValue(true));
        }
    }
}

