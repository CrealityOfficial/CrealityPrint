#include "mirroractioncommand.h"

#include "interface/selectorinterface.h"
#include "interface/commandinterface.h"

namespace creative_kernel
{
    MirrorActionCommand::MirrorActionCommand(MirrorOperation operation, QObject* parent)
        : ActionCommand(parent)
        , m_operation(operation)
    {
        m_eParentMenu = eMenuType_View;
        addUIVisualTracer(this,this);
    }

    MirrorActionCommand::~MirrorActionCommand()
    {
    }

    void MirrorActionCommand::execute()
    {
        switch (m_operation)
        {
        case MirrorOperation::mo_x:
            mirrorX();
            break;
        case MirrorOperation::mo_y:
            mirrorY();
            break;
        case MirrorOperation::mo_z:
            mirrorZ();
            break;
        case MirrorOperation::mo_reset:
            reset();
            break;
        }
    }

    void MirrorActionCommand::mirrorX()
    {
        creative_kernel::mirrorXSelections(true);
    }

    void MirrorActionCommand::mirrorY()
    {
        creative_kernel::mirrorYSelections(true);
    }

    void MirrorActionCommand::mirrorZ()
    {
        creative_kernel::mirrorZSelections(true);
    }

    void MirrorActionCommand::reset()
    {
    }

    void MirrorActionCommand::onThemeChanged(ThemeCategory category)
    {

    }

    void MirrorActionCommand::onLanguageChanged(MultiLanguage language)
    {
        switch (m_operation)
        {
        case MirrorOperation::mo_x:
            m_actionNameWithout = "Mirrror X";
            m_actionname = tr("Mirrror X");
            break;
        case MirrorOperation::mo_y:
            m_actionNameWithout = "Mirrror Y";
            m_actionname = tr("Mirrror Y");
            break;
        case MirrorOperation::mo_z:
            m_actionNameWithout = "Mirrror Z";
            m_actionname = tr("Mirrror Z");
            break;
        case MirrorOperation::mo_reset:
            m_actionNameWithout = "Mirrror ReSet";
            m_actionname = tr("Mirrror ReSet");
            break;
        }
    }
}

