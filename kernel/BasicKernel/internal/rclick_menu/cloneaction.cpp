#include "cloneaction.h"
#include "interface/selectorinterface.h"
#include "interface/commandinterface.h"
#include "cxkernel/interface/jobsinterface.h"
#include "interface/uiinterface.h"
#include "job/nest2djob.h"

namespace creative_kernel
{
    CloneAction::CloneAction(QObject* parent)
        :ActionCommand(parent)
    {
        m_actionname = tr("Clone Model");
        m_actionNameWithout = "Clone Model";

        addUIVisualTracer(this,this);
    }

    CloneAction::~CloneAction()
    {
    }

    void CloneAction::onThemeChanged(ThemeCategory category)
    {
    }

    void CloneAction::onLanguageChanged(MultiLanguage language)
    {
        m_actionname = tr("Clone Model");
    }

    bool CloneAction::isSelect()
    {
        QList<ModelN*> selections = selectionms();
        if (selections.size() > 0)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    void CloneAction::clone(int numb)
    {
        cmdClone(numb);
        //creative_kernel::Nest2DJob* job = new Nest2DJob(this);
        //QList<qtuser_core::JobPtr> jobs;
        //jobs.push_back(qtuser_core::JobPtr(job));
        //executeJobs(jobs);
    }

    void CloneAction::execute()
    {
        requestQmlDialog(this, "cloneNumObj");
    }

    bool CloneAction::enabled()
    {
        return isSelect();
    }
}
