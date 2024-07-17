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
#include "interface/spaceinterface.h"

namespace creative_kernel
{
    ProjectInfoUI* ProjectInfoUI::m_info=nullptr;
    ProjectInfoUI::ProjectInfoUI(QObject* parent)
        : QObject(parent)
        , m_callback(nullptr)
    {
        m_spaceDirty = false;
        QString tmpProject = qtuser_core::getOrCreateAppDataLocation("tmpProject");
        QDir tempDir;
        if (!tempDir.exists(tmpProject))
        {
            tempDir.mkpath(tmpProject);
        }
        
        m_strTmpCacheProjectPath = ":/cx3d/cx3d_exporter/NewProject.cxprj";
        
        QSettings setting;
        setting.beginGroup("AutoSave_Path");
        m_strProjectName = "NewProject.cxprj";
        m_strProjectPath = m_strTmpCacheProjectPath;
        m_strMessageText = tr("Do you Want to Save LastProject?");

        QString strFileName = setting.value("filePath", "default").toString();
        setting.endGroup();
        if (!strFileName.contains("default"))
        {
            m_strTempFilePath = strFileName;
            m_strProjectName = m_strTempFilePath.mid(m_strTempFilePath.lastIndexOf("/")+1);
        }
        else {
            m_strTempFilePath = qtuser_core::getOrCreateAppDataLocation("tmpProject") + "/NewProject.cxprj";
        }
        setSettingsData(m_strTempFilePath);

        addUIVisualTracer(this,this);
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
    void ProjectInfoUI::setSpaceDirty(bool _spaceDirty)
    {
        if(modelns().size() == 0)
            _spaceDirty = false;
        if (m_spaceDirty == _spaceDirty)return;
        m_spaceDirty = _spaceDirty;
        emit spaceDirtyChanged();
    }

    bool ProjectInfoUI::spaceDirty()
    {
        return m_spaceDirty;
    }

    QString ProjectInfoUI::getProjectNameNoSuffix()
    {
        int index = m_strProjectName.lastIndexOf(".");
        return m_strProjectName.left(index);
    }
    bool ProjectInfoUI::is3mf()
    {
        QFileInfo file(m_strProjectPath);
        if (file.exists())
        {
            return QString(file.suffix()).toLower() == "3mf";
        }
        return false;
    }
    QString ProjectInfoUI::getProjectName()
    {
        return m_strProjectName;
    }

    void ProjectInfoUI::setProjectName(QString strProName)
    {
        if(strProName == m_strProjectName)
            return;
        m_strProjectName = strProName;
        if(QFileInfo(m_strProjectPath).exists())
        {
            QDir dir(m_strTempFilePath);
            dir.rename(m_strTempFilePath,qtuser_core::getOrCreateAppDataLocation("tmpProject") + "/" + m_strProjectName);
        }
            
        m_strTempFilePath = qtuser_core::getOrCreateAppDataLocation("tmpProject") + "/" + m_strProjectName;
        setSettingsData(m_strTempFilePath);
        emit projectNameChanged();
    }

    QString ProjectInfoUI::getProjectPath()
    {
        return m_strProjectPath;
    }

    void ProjectInfoUI::setProjectPath(QString strProPath)
    {
        m_strProjectPath = strProPath;
        emit projectNameChanged();
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

    void ProjectInfoUI::clearSecen(bool clearModel)
    {
        using namespace creative_kernel;
        clearProjectCache(clearModel);
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
    bool ProjectInfoUI::availablePath()
    {

        QFile file(m_strProjectPath);
        if (file.exists() && m_strProjectPath != m_strTmpCacheProjectPath) return true;
        return false;
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

