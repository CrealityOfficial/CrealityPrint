#include "projectinfoui.h"
#include <QtCore/QSettings>
#include <QtCore/QFileInfo>
#include <QtQml/QQmlProperty>

#include "kernel/kernelui.h"
#include "interface/modelinterface.h"
#include "interface/commandinterface.h"
#include "interface/uiinterface.h"

namespace creative_kernel
{
    ProjectInfoUI* ProjectInfoUI::m_info=nullptr;
    ProjectInfoUI::ProjectInfoUI(QObject* parent)
        :QObject(parent)
    {
        QSettings setting;
        //    setting.beginGroup("AutoSave_Path");
        //    setting.setValue("saveminutes", "5.0");
        //    setting.endGroup();

        setting.beginGroup("AutoSave_Path");
        m_fMinutes = setting.value("saveminutes", "10.0").toFloat();
        setting.endGroup();
        //    m_fMinutes = 5.0;
        m_strProjectName = "";
        m_strProjectPath = "";
        m_strMessageText = tr("Do you Want to Save LastProject?");

        addUIVisualTracer(this);
    }

    ProjectInfoUI::~ProjectInfoUI()
    {

    }
    ProjectInfoUI* ProjectInfoUI::instance()
    {
        //static ProjectInfoUI info;
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

    //default m_bAcceptDialog = false
    void ProjectInfoUI::requestMenuDialog()
    {
        m_bAcceptDialog = false;
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

    //when qml yes ,m_bAcceptDialog = true
    void ProjectInfoUI::accept()
    {
        emit acceptDialog();
    }
    void ProjectInfoUI::cancel()
    {

        emit cancelDialog();
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

