#include "mergemodeltoprinteraction.h"

#include "interface/selectorinterface.h"
#include "cxkernel/interface/jobsinterface.h"
#include "interface/commandinterface.h"
#include "interface/spaceinterface.h"
#include "job/splitmodeljob.h"
#include "interface/printerinterface.h"
#include "internal/multi_printer/printer.h"

namespace creative_kernel
{
    MergeModelToPrinterAction::MergeModelToPrinterAction(QObject* parent)
        : ActionCommand(parent)
    {
        m_actionname = tr("Merge Model Location");
        m_actionNameWithout = "Unit As One";
        setShortcut("Ctrl+M");
        addUIVisualTracer(this,this);
    }

    MergeModelToPrinterAction::~MergeModelToPrinterAction()
    {
    }

    void MergeModelToPrinterAction::execute()
    {
        auto models = selectionms();
        auto box = getSelectedPrinter()->globalBox();
    }

    bool MergeModelToPrinterAction::enabled()
    {
        return selectionms().size() > 0;
    }

    void MergeModelToPrinterAction::onThemeChanged(ThemeCategory category)
    {
    }

    void MergeModelToPrinterAction::onLanguageChanged(MultiLanguage language)
    {
        m_actionname = tr("Merge Model Location");
    }
}
