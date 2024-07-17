#include "mergemodellocation.h"

#include "interface/modelinterface.h"
#include "interface/selectorinterface.h"
#include "cxkernel/interface/jobsinterface.h"
#include "interface/commandinterface.h"
#include "job/splitmodeljob.h"
#include "interface/printerinterface.h"
#include "internal/multi_printer/printer.h"
namespace creative_kernel
{
    MergeModelLocation::MergeModelLocation(QObject* parent)
        : ActionCommand(parent)
    {
        m_actionname = tr("Merge Model Location");
        m_actionNameWithout = "Unit As One";

        addUIVisualTracer(this,this);
    }

    MergeModelLocation::~MergeModelLocation()
    {
    }

    void MergeModelLocation::execute()
    {
        auto models = selectionms();
        auto box = getSelectedPrinter()->globalBox();

        mergePosition(models, box, true);
    }

    bool MergeModelLocation::enabled()
    {
        return selectionms().size() > 0;
    }

    void MergeModelLocation::onThemeChanged(ThemeCategory category)
    {
    }

    void MergeModelLocation::onLanguageChanged(MultiLanguage language)
    {
        m_actionname = tr("Merge Model Location");
    }
}
