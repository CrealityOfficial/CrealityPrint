#include "openfilecommand.h"
#include "interface/commandinterface.h"
#include "interface/spaceinterface.h"
#include "cxkernel/interface/iointerface.h"
#include "interface/selectorinterface.h"
#include <QCoreApplication>

namespace creative_kernel
{
    OpenFileCommand::OpenFileCommand()
        :ActionCommand()
    {
        m_shortcut = "Ctrl+I";
        m_actionNameWithout = "Model File";
        m_actionname = QCoreApplication::translate("MenuCmdObj", m_actionNameWithout.toLatin1()); //tr("Model File");
        m_icon = "qrc:/kernel/images/open.png";
        m_eParentMenu = eMenuType_File;

        addUIVisualTracer(this,this);
    }

    OpenFileCommand::~OpenFileCommand()
    {
    }

    void OpenFileCommand::setFileType(OpenType type)
    {
        m_openType = type;
    }

    void OpenFileCommand::setJoinGroupEnabled(bool enabled)
    {
        m_joinGroupEnabled = enabled;
    }

    void OpenFileCommand::setModelType(ModelNType type)
    {
        m_partType = type;
    }

    void OpenFileCommand::execute()
    {
        setKernelPhase(KernelPhaseType::kpt_prepare);

        ModelGroup* group = NULL;
        if (m_joinGroupEnabled)
        {
            QList<ModelGroup*> modelGroups = selectedGroups();
            group = modelGroups.first();
        }
        setOpenedModelType(m_partType, group);

        if (m_openType == All)
            cxkernel::openFile();
        else if (m_openType == Mesh)
            openMeshFile();
    }

    void OpenFileCommand::onThemeChanged(ThemeCategory category)
    {
    }

    void OpenFileCommand::onLanguageChanged(MultiLanguage language)
    {
        m_actionname = QCoreApplication::translate("MenuCmdObj", m_actionNameWithout.toLatin1());
    }
}
