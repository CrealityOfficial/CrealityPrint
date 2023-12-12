#include "splitmodelaction.h"

#include "data/modeln.h"

#include "interface/modelinterface.h"
#include "interface/selectorinterface.h"
#include "cxkernel/interface/jobsinterface.h"
#include "interface/commandinterface.h"
#include "job/splitmodeljob.h"

namespace creative_kernel
{
    SplitModelAction::SplitModelAction(QObject* parent)
        : ActionCommand(parent)
    {
        m_actionname = tr("Split Model");
        m_actionNameWithout = "Split Model";

        addUIVisualTracer(this);
    }

    SplitModelAction::~SplitModelAction()
    {
    }

    void SplitModelAction::execute()
    {
        SplitModelJob* job = new SplitModelJob();
        job->EnableSplit(true);

        job->setModels(selectionms());
        cxkernel::executeJob(qtuser_core::JobPtr(job));
    }

    bool SplitModelAction::enabled()
    {
        return selectionms().size() > 0;
    }

    void SplitModelAction::onThemeChanged(ThemeCategory category)
    {

    }

    void SplitModelAction::onLanguageChanged(MultiLanguage language)
    {
        m_actionname = tr("Split Model");
    }
}


