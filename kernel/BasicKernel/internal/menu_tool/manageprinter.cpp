#include "manageprinter.h"

#include "interface/commandinterface.h"
#include "interface/uiinterface.h"

namespace creative_kernel
{
    ManagePrinter::ManagePrinter(QObject* parent)
        :ActionCommand(parent)
    {
        m_actionname = tr("Manage Printer");
        m_actionNameWithout = "Manage Printer";
        m_eParentMenu = eMenuType_Tool;

        addUIVisualTracer(this);
    }

    ManagePrinter::~ManagePrinter()
    {

    }

    void ManagePrinter::execute()
    {
        invokeMainWindowFunc("showManagePrinterDialog");
    }

    void ManagePrinter::onThemeChanged(ThemeCategory category)
    {

    }

    void ManagePrinter::onLanguageChanged(MultiLanguage language)
    {
        m_actionname = tr("Manage Printer");
    }
}


