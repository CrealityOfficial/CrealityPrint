#include "selectprinterallaction.h"

#include "interface/selectorinterface.h"
#include "interface/spaceinterface.h"
#include "interface/commandinterface.h"
#include "interface/printerinterface.h"
#include "internal/multi_printer/printer.h"

namespace creative_kernel
{
    SelectPrinterAllAction::SelectPrinterAllAction(QObject* parent)
        : ActionCommand(parent)
    {
        m_actionname = tr("Select All Model");
        m_actionNameWithout = "Select All Model";

        addUIVisualTracer(this,this);
    }

    SelectPrinterAllAction::~SelectPrinterAllAction()
    {
    }

    void SelectPrinterAllAction::execute()
    {
        Printer* printer = getSelectedPrinter();

        auto insideModels = printer->modelGroupsInside();
        auto onBorderModels = printer->modelGroupsOnBorder();
        selectGroups(insideModels + onBorderModels);
    }

    bool SelectPrinterAllAction::enabled()
    {
        Printer* printer = getSelectedPrinter();
        if (!printer)
            return false;

        auto insideModels = printer->modelGroupsInside();
        auto onBorderModels = printer->modelGroupsOnBorder();

        return insideModels.size() + onBorderModels.size() > 0;
    }

    void SelectPrinterAllAction::onThemeChanged(ThemeCategory category)
    {
    }

    void SelectPrinterAllAction::onLanguageChanged(MultiLanguage language)
    {
        m_actionname = tr("Select All Model");
    }
}
