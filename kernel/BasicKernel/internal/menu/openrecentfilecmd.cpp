#include "openrecentfilecmd.h"

#include "submenurecentfiles.h"
#include "interface/uiinterface.h"
#include "cxkernel/interface/iointerface.h"
#include "data/kernelenum.h"
#include "external/interface/commandinterface.h"
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
        cxkernel::openFileWithString(m_actionname);
        SubMenuRecentFiles::getInstance()->setMostRecentFile(m_actionname);
    }
}
