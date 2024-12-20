#include "mergemodelaction.h"

#include "interface/selectorinterface.h"
#include "cxkernel/interface/jobsinterface.h"
#include "interface/commandinterface.h"
#include "job/splitmodeljob.h"

namespace creative_kernel
{
    MergeModelAction::MergeModelAction(QObject* parent)
        : ActionCommand(parent)
    {
        // m_actionname = tr("Merge Model");
        // m_actionNameWithout = "Merge Model";
        m_actionname = tr("Reset Model Location");
        m_actionNameWithout = "Reset Model Location";

        addUIVisualTracer(this,this);
    }

    MergeModelAction::~MergeModelAction()
    {
    }

    void MergeModelAction::execute()
    {
        SplitModelJob* job = new SplitModelJob();

        job->EnableSplit(false);
        job->setModelGroups(selectedGroups());
        cxkernel::executeJob(qtuser_core::JobPtr(job));
    }

    bool MergeModelAction::enabled()
    {
        return selectedGroups().size() > 0;
    }
    void MergeModelAction::onThemeChanged(ThemeCategory category)
    {
    }

    void MergeModelAction::onLanguageChanged(MultiLanguage language)
    {
        m_actionname = tr("Reset Model Location");
    }
}
