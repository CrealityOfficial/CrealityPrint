#include "mergemodellocation.h"

#include "interface/selectorinterface.h"
#include "cxkernel/interface/jobsinterface.h"
#include "interface/commandinterface.h"
#include "interface/spaceinterface.h"
#include "job/splitmodeljob.h"
#include "interface/printerinterface.h"
#include "internal/multi_printer/printer.h"
namespace creative_kernel
{
    MergeModelLocation::MergeModelLocation(QObject* parent)
        : ActionCommand(parent)
    {
        m_actionname = tr("Reset Model Location");
        m_actionNameWithout = "Reset Model Location";

        addUIVisualTracer(this,this);
    }

    MergeModelLocation::~MergeModelLocation()
    {
    }

    void MergeModelLocation::execute()
    {
        auto box = getSelectedPrinter()->globalBox();

        trimesh::dbox3 b;
        b += trimesh::dvec3(box.max.x(), box.max.y(), box.max.z());
        b += trimesh::dvec3(box.min.x(), box.min.y(), box.min.z());
        mergePosition(selectedGroups(), b, true);
    }

    bool MergeModelLocation::enabled()
    {
        int selGroupCount = selectedGroups().size();
        if (selGroupCount <= 1)
            return false;

        for (int i = 0; i < selGroupCount; i++)
        {
            if (selectedGroups()[i]->models().size() > 1)
                return false;
        }

        return true;
    }

    void MergeModelLocation::onThemeChanged(ThemeCategory category)
    {
    }

    void MergeModelLocation::onLanguageChanged(MultiLanguage language)
    {
        m_actionname = tr("Reset Model Location");
    }
}
