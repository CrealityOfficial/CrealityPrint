#include "clearprinteraction.h"
#include "kernel/kernelui.h"
#include "interface/modelinterface.h"
#include "interface/spaceinterface.h"
#include "interface/commandinterface.h"
#include "interface/uiinterface.h"
#include "interface/printerinterface.h"
#include "internal/multi_printer/printer.h"

namespace creative_kernel
{
    ClearPrinterAction::ClearPrinterAction(QObject* parent):ActionCommand(parent)
    {
        m_actionname = tr("Clear All");
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

        auto insideModels = printer->modelsInside();
        auto onBorderModels = printer->modelsOnBorder();

        QList<ModelN*> adds;

        modifySpace(insideModels + onBorderModels, adds, true);
    }

    void ClearPrinterAction::onThemeChanged(ThemeCategory category)
    {
    }

    void ClearPrinterAction::onLanguageChanged(MultiLanguage language)
    {
        m_actionname = tr("Clear All");
    }

    bool ClearPrinterAction::enabled()
    {
        Printer* printer = getSelectedPrinter();
        if (!printer)
            return false;

        auto insideModels = printer->modelsInside();
        auto onBorderModels = printer->modelsOnBorder();

        return insideModels.size() + onBorderModels.size() > 0;
    }
}
