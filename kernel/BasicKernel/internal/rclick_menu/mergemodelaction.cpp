#include "mergemodelaction.h"

#include "interface/modelinterface.h"
#include "interface/selectorinterface.h"
#include "cxkernel/interface/jobsinterface.h"
#include "interface/commandinterface.h"
#include "job/splitmodeljob.h"

namespace creative_kernel
{
    MergeModelAction::MergeModelAction(QObject* parent)
        : ActionCommand(parent)
    {
        m_actionname = tr("Merge Model");
        m_actionNameWithout = "Merge Model";

        addUIVisualTracer(this,this);
    }

    MergeModelAction::~MergeModelAction()
    {
    }

    void MergeModelAction::execute()
    {
        SplitModelJob* job = new SplitModelJob();

        job->EnableSplit(false);
        job->setModels(selectionms());
        cxkernel::executeJob(qtuser_core::JobPtr(job));
    }

    bool MergeModelAction::enabled()
    {
        return selectionms().size() > 0;
    }

    void MergeModelAction::onThemeChanged(ThemeCategory category)
    {
    }

    void MergeModelAction::onLanguageChanged(MultiLanguage language)
    {
        m_actionname = tr("Merge Model");
    }
}
