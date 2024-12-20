#include "clearprinteraction.h"
#include "kernel/kernelui.h"
#include "interface/spaceinterface.h"
#include "interface/commandinterface.h"
#include "interface/uiinterface.h"
#include "interface/printerinterface.h"
#include "internal/multi_printer/printer.h"
#include "buildinfo.h"
#include <QCoreApplication>

namespace creative_kernel
{
    ClearPrinterAction::ClearPrinterAction(QObject* parent):ActionCommand(parent)
    {
        m_actionname = QCoreApplication::translate(TRANSLATE_CONTEXT, "Clear All Models");
        m_actionNameWithout = "Clear All";
        m_source = "qrc:/UI/photo/menuImg/delete_all_n.svg";
        m_icon = "qrc:/UI/photo/menuImg/delete_all_p.svg";

        addUIVisualTracer(this,this);
    }

    ClearPrinterAction::~ClearPrinterAction()
    {
    }

    void ClearPrinterAction::execute()
    {
        Printer* printer = getSelectedPrinter();
        if (!printer)
            return;

        auto insideModels = printer->modelGroupsInside();
        auto onBorderModels = printer->modelGroupsOnBorder();

        deleteModelGroups(insideModels + onBorderModels);
    }

    void ClearPrinterAction::onThemeChanged(ThemeCategory category)
    {
    }

    void ClearPrinterAction::onLanguageChanged(MultiLanguage language)
    {
        m_actionname = QCoreApplication::translate(TRANSLATE_CONTEXT, "Clear All Models");
    }

    bool ClearPrinterAction::enabled()
    {
        Printer* printer = getSelectedPrinter();
        if (!printer)
            return false;

        auto insideModels = printer->modelGroupsInside();
        auto onBorderModels = printer->modelGroupsOnBorder();

        return insideModels.size() + onBorderModels.size() > 0;
    }
}
