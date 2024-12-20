#include "splitmodelaction.h"

#include "data/modeln.h"

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
        m_source = "qrc:/UI/photo/menuImg/split_model_n.svg";
        m_icon = "qrc:/UI/photo/menuImg/split_model_p.svg";

        addUIVisualTracer(this,this);
    }

    SplitModelAction::~SplitModelAction()
    {
    }

    void SplitModelAction::execute()
    {
        SplitModelJob* job = new SplitModelJob();
        job->EnableSplit(true);
        job->setModelGroups(selectedGroups());
        cxkernel::executeJob(qtuser_core::JobPtr(job));
    }

    bool SplitModelAction::enabled()
    {
        return selectedGroups().size() == 1;
    }

    void SplitModelAction::onThemeChanged(ThemeCategory category)
    {

    }

    void SplitModelAction::onLanguageChanged(MultiLanguage language)
    {
        m_actionname = tr("Split Model");
    }
}


