#include "openfilecommand.h"
#include "interface/modelinterface.h"
#include "interface/commandinterface.h"
#include "cxkernel/interface/iointerface.h"
#include <QCoreApplication>
namespace creative_kernel
{
    OpenFileCommand::OpenFileCommand()
        :ActionCommand()
    {
        m_shortcut = "Ctrl+I";
        m_actionname = QCoreApplication::translate("MenuCmdObj", "Model File"); //tr("Model File");
        m_actionNameWithout = "Open File";
        m_icon = "qrc:/kernel/images/open.png";
        m_eParentMenu = eMenuType_File;

        addUIVisualTracer(this,this);
    }

    OpenFileCommand::~OpenFileCommand()
    {
    }

    void OpenFileCommand::execute()
    {
        setKernelPhase(KernelPhaseType::kpt_prepare);
        cxkernel::openFile();
    }

    void OpenFileCommand::onThemeChanged(ThemeCategory category)
    {
    }

    void OpenFileCommand::onLanguageChanged(MultiLanguage language)
    {
        m_actionname = QCoreApplication::translate("MenuCmdObj", "Model File");
    }
}
