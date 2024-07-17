#include "coverdModelAction.h"
#include "interface/layoutinterface.h"
#include "interface/selectorinterface.h"
#include "interface/printerinterface.h"
#include "interface/commandinterface.h"
#include "internal/multi_printer/printer.h"

namespace creative_kernel
{

    CoverModelAction::CoverModelAction(QObject* parent) 
    {
        m_actionname = tr("Fill bed with copies");
        m_actionNameWithout = "Fill bed with copies";

        addUIVisualTracer(this,this);
    }

    CoverModelAction::~CoverModelAction()
    {

    }

    void CoverModelAction::execute()
    {
        QList<ModelN*> selects = selectionms();
        if (selects.count() > 1 || selects.count() == 0)
            return;

        int printerIndex = getModelPrinterIndex(selects.at(0));
        selectPrinter(printerIndex);
        extendFillModel(selects.at(0), printerIndex);
    }

    bool CoverModelAction::enabled()
    {
        QList<ModelN*> selects = selectionms();
        if (selects.count() > 1 || selects.count() == 0)
            return false;

        ModelN* model = selects.at(0);

        for (int i = 0, count = printersCount(); i < count; ++i)
        {
            Printer* printer = getPrinter(i);
            if (printer->modelsInside().contains(model))
                return true;
        }
        return false;
    }

    void CoverModelAction::onThemeChanged(ThemeCategory category)
    {
        
    }

    void CoverModelAction::onLanguageChanged(MultiLanguage language)
    {
        m_actionname = tr("Fill bed with copies");
    }

}