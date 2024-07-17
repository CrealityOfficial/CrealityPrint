#include "loadprojectcommand.h"

#include <QtCore/QFileInfo>

#include "kernel/kernelui.h"
#include "cxkernel/interface/jobsinterface.h"
#include "cx3dexportjob.h"
#include "cx3dautosaveproject.h"

#include "interface/uiinterface.h"
#include "interface/projectinterface.h"
#include <kernel/kernel.h>
#include <QFileDialog>
#include <cxkernel/interface/iointerface.h>
#include <qtusercore/util/settings.h>
#include <QCoreApplication>
LoadProjectCommand::LoadProjectCommand(QObject* parent)
    : ActionCommand(parent)
{
}

LoadProjectCommand::~LoadProjectCommand()
{
}

void LoadProjectCommand::accept()
{
    QString savePath = creative_kernel::projectUI()->getProjectPath();
    if(!creative_kernel::projectUI()->availablePath())
    {
        savePath = saveAsDialogPath();
    }

    saveProject(savePath);
    loadProject(m_willOpenProject);
}

void LoadProjectCommand::cancel()
{
    loadProject(m_willOpenProject);
}

void LoadProjectCommand::openProject(const QString& fileName)
{
    m_willOpenProject = fileName;

    QString strOldPath = creative_kernel::projectUI()->getProjectPath();
    if (creative_kernel::projectUI()->spaceDirty() && !strOldPath.isEmpty() && strOldPath != m_willOpenProject)
    {
        creative_kernel::projectUI()->requestMenuDialog(this);
    }
    else
    {
        loadProject(m_willOpenProject);
    }
}
QString LoadProjectCommand::saveAsDialogPath()
{
    if (creative_kernel::projectUI()->availablePath())
        QStringList fileNames;
    auto f = [this](const QString& file) {
        cxkernel::openFileWithName(file);
    };

    QString title = QCoreApplication::translate("CustomObject", "Save Project As");
    QString filter = "Project file(*.cxprj *.3mf)";
    qtuser_core::VersionSettings setting;
    QString lastPath = setting.value("dialogLastPath", "").toString();

#ifdef Q_OS_LINUX
    QString fileName = QFileDialog::getSaveFileName(
        nullptr, title,
        lastPath, filter, nullptr, QFileDialog::DontUseNativeDialog);
#else
    QString fileName = QFileDialog::getSaveFileName(
        nullptr, title,
        lastPath, filter);
#endif
    if (fileName.isEmpty())
        return "";
    QFileInfo fileinfo = QFileInfo(fileName);
    lastPath = fileinfo.path();
    setting.setValue("dialogLastPath", lastPath);
    return fileName;
}
