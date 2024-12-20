#include "openrecentfilecmd.h"
#include "data/kernelenum.h"
#include "interface/uiinterface.h"
#include "interface/commandinterface.h"
#include "interface/spaceinterface.h"

#include <QtCore/QFileInfo>

namespace creative_kernel
{
    OpenRecentFileCmd::OpenRecentFileCmd()
        :ActionCommand()
    {
        m_actionname = "";
        m_eParentMenu = eMenuType_File;
    }

    OpenRecentFileCmd::~OpenRecentFileCmd()
    {
    }

    void OpenRecentFileCmd::execute()
    {
        setKernelPhase(KernelPhaseType::kpt_prepare);
        QFileInfo fileInfo(m_actionname);
        if (!fileInfo.exists())
        {
            requestQmlDialog(this, "openRecentlyFileDlg");
            return;
        } 

        openSpecifyMeshFile(m_actionname);
    }
}
