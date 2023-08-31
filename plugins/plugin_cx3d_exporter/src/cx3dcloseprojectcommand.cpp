#include "cx3dcloseprojectcommand.h"
#include "kernel/projectinfoui.h"
#include "cx3dsubmenurecentproject.h"
#include "cxkernel/interface/jobsinterface.h"
#include "interface/commandinterface.h"
#include "interface/uiinterface.h"
#include "cxkernel/interface/iointerface.h"
#include "cx3dexportjob.h"
#include "cx3dexporter.h"
#include "kernel/kernel.h"
#include "kernel/kernelui.h"
#include "qtusercore/string/resourcesfinder.h"
#include "interface/spaceinterface.h"

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

        addUIVisualTracer(this);

        disconnect(getKernelUI(), SIGNAL(closeWindow()), this, SLOT(slotCloseWindow()));
        connect(getKernelUI(), SIGNAL(closeWindow()), this, SLOT(slotCloseWindow()));
    }

    Cx3dCloseProjectCommand::~Cx3dCloseProjectCommand()
    {
    }

    void Cx3dCloseProjectCommand::execute()
    {
        requestMenuDialog();
    }

    void Cx3dCloseProjectCommand::requestMenuDialog()
    {
        requestQmlDialog(this, "closeProfile");
    }

    void Cx3dCloseProjectCommand::saveFile()
    {
        saveProPath();
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
        qtuser_core::JobPtr job(new Cx3dExportJob(fileName, Cx3dExportJob::JobType::WRITEJOB));
        cxkernel::executeJob(job);
        QSettings settings;
        Cx3dSubmenuRecentProject::getInstance()->setMostRecentFile(fileName);
        disconnect(getKernel()->jobExecutor(), SIGNAL(jobsEnd()), this, SLOT(finishJob()));
        connect(getKernel()->jobExecutor(), SIGNAL(jobsEnd()), this, SLOT(finishJob()));

        delDefaultProject();
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
        CX3DExporter* exporter = qobject_cast<CX3DExporter*>(parent());
        return exporter->openedPath();
    }

    void Cx3dCloseProjectCommand::delDefaultProject()
    {
        QString pathDir = qtuser_core::getOrCreateAppDataLocation("") + "/tmpProject";
        QDir tempDir(pathDir);
        tempDir.removeRecursively();
    }

    void Cx3dCloseProjectCommand::finishJob()
    {
        requestQmlCloseAction();
    }

    void Cx3dCloseProjectCommand::slotCloseWindow()
    {
        requestMenuDialog();
    }
}