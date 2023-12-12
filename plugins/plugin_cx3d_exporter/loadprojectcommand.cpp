#include "loadprojectcommand.h"

#include <QtCore/QFileInfo>

#include "kernel/kernelui.h"
#include "cxkernel/interface/jobsinterface.h"
#include "cx3dexportjob.h"
#include "cx3dautosaveproject.h"

#include "interface/uiinterface.h"
#include "interface/projectinterface.h"

LoadProjectCommand::LoadProjectCommand(QObject* parent)
    : ActionCommand(parent)
{
}

LoadProjectCommand::~LoadProjectCommand()
{
}

void LoadProjectCommand::accept()
{
    saveProject(creative_kernel::projectUI()->getAutoProjectPath());
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
    if (!strOldPath.isEmpty() && strOldPath != m_willOpenProject)
    {
        creative_kernel::projectUI()->requestMenuDialog(this);
    }
    else
    {
        loadProject(m_willOpenProject);
    }
}
