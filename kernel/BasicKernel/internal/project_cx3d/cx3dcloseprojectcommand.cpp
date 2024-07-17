#include "cx3dcloseprojectcommand.h"
#include "cxkernel/interface/jobsinterface.h"
#include "interface/commandinterface.h"
#include "interface/uiinterface.h"
#include "cxkernel/interface/iointerface.h"
#include "cx3dexportjob.h"
#include "cx3dmanager.h"
#include "kernel/kernelui.h"

#include "interface/spaceinterface.h"
#include "interface/projectinterface.h"

#include <QtCore/QDir>

namespace creative_kernel
{
    Cx3dCloseProjectCommand::Cx3dCloseProjectCommand(QObject* parent)
        : ActionCommand(parent)
    {
        m_shortcut = "Ctrl+Q";      //不能有空格
        m_actionname = tr("Close");//
        m_actionNameWithout = "Close";
        m_icon = "qrc:/kernel/images/new.png";
        m_eParentMenu = eMenuType_File;

        addUIVisualTracer(this,this);
    }

    Cx3dCloseProjectCommand::~Cx3dCloseProjectCommand()
    {
    }

    void Cx3dCloseProjectCommand::execute()
    {
        QMetaObject::invokeMethod(getUIAppWindow(), "closeWindow");    
    }

    void Cx3dCloseProjectCommand::requestMenuDialog()
    {
        requestQmlDialog(this, "closeProfile");
    }

    void Cx3dCloseProjectCommand::saveFile(bool bExit)
    {
        saveProPath();
        delDefaultProject();
        if(bExit)
        {
            creative_kernel::projectUI()->clearSecen();
        }
    }
    void Cx3dCloseProjectCommand::clearScreen()
    {
        creative_kernel::projectUI()->clearSecen();
        delDefaultProject();
    }
    void Cx3dCloseProjectCommand::noSaveFile()
    {
        requestQmlCloseAction();
    }

    void Cx3dCloseProjectCommand::saveProPath()
    {
        cxkernel::saveFile(this, QString("%1.cxprj").arg(mainModelName()));
    }

    QString Cx3dCloseProjectCommand::filter()
    {
        QString _filter = "Project File (*.cxprj)";
        return _filter;
    }

    void Cx3dCloseProjectCommand::handle(const QString& fileName)
    {
        delDefaultProject();
        saveProject(fileName);
    }

    void Cx3dCloseProjectCommand::onThemeChanged(ThemeCategory category)
    {
    }

    void Cx3dCloseProjectCommand::onLanguageChanged(MultiLanguage language)
    {
        m_actionname = tr("Close") + "        " + m_shortcut;
    }

    QUrl Cx3dCloseProjectCommand::historyPath()
    {
        CX3DManager* exporter = qobject_cast<CX3DManager*>(parent());
        return exporter->openedPath();
    }

    void Cx3dCloseProjectCommand::delDefaultProject()
    {
        creative_kernel::projectUI()->deleteTempProjectDirectory();
    }

    void Cx3dCloseProjectCommand::onWindowClose()
    {
        requestMenuDialog();
    }
}