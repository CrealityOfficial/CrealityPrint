#include "importmodelaction.h"

#include "interface/commandinterface.h"
#include "cxkernel/interface/iointerface.h"

namespace creative_kernel
{
    ImportModelAction::ImportModelAction(QObject *parent)
        :ActionCommand(parent)
    {
        m_actionname = tr("Import Model");
        m_actionNameWithout = "Import Model";
        m_icon = "qrc:/kernel/images/open.png";

        addUIVisualTracer(this,this);
    }

    ImportModelAction::~ImportModelAction()
    {
    }

    void ImportModelAction::execute()
    {
        cxkernel::openFile();
    }

    bool ImportModelAction::enabled()
    {
        return true;
    }

    void ImportModelAction::onThemeChanged(ThemeCategory category)
    {
    }

    void ImportModelAction::onLanguageChanged(MultiLanguage language)
    {
        m_actionname = tr("Import Model");
    }
}
