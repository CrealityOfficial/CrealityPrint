#include "cx3drecentprojectcommand.h"

#include <QtCore/QFileInfo>

#include "kernel/kernelui.h"
#include "interface/uiinterface.h"

using namespace creative_kernel;
Cx3dRecentProjectCommand::Cx3dRecentProjectCommand(QObject* parent)
    : LoadProjectCommand(parent)
{
    m_actionname = "";
    m_eParentMenu = eMenuType_File;
}

Cx3dRecentProjectCommand::~Cx3dRecentProjectCommand()
{
}

void Cx3dRecentProjectCommand:: execute()
{
    m_actionname = m_actionname.replace("file:///", "");//去掉前缀

    QFileInfo fileInfo(m_actionname);
    if (!fileInfo.exists())
    {
        requestQmlDialog(this, "openRecentlyFileDlg");
        return;
    }

    openProject(m_actionname);
}

void Cx3dRecentProjectCommand::accept()
{
}

void Cx3dRecentProjectCommand::cancel()
{
}
