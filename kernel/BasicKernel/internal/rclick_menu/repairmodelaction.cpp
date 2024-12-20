#include "repairmodelaction.h"

#include "interface/selectorinterface.h"
#include "cxkernel/interface/jobsinterface.h"
#include "interface/commandinterface.h"
#include "interface/uiinterface.h"
#include "kernel/kernel.h"
#include "external/job/modelrepairob.h"
#include "crslice2/repair.h"

namespace creative_kernel
{
    RepairModelAction::RepairModelAction(QObject* parent)
        : ActionCommand(parent)
    {
        m_actionname = tr("Repair Model");
        m_actionNameWithout = "Repair Model";

        addUIVisualTracer(this,this);
    }

    RepairModelAction::~RepairModelAction()
    {
    }

    QString RepairModelAction::getMessageText()
    {
        return tr("Repair Model Finish!!!");
    }

    void RepairModelAction::execute()
    {
		ModelRepairWinJob* job = new ModelRepairWinJob();

		cxkernel::executeJob(qtuser_core::JobPtr(job));
		disconnect(getKernel()->jobExecutor(), SIGNAL(jobsEnd()), this, SIGNAL(repairSuccess()));
		connect(getKernel()->jobExecutor(), SIGNAL(jobsEnd()), this, SIGNAL(repairSuccess()));

		connect(job, &ModelRepairWinJob::repairSuccessed, this, &RepairModelAction::slotRepairSuccess);
    }

    bool RepairModelAction::enabled()
    {
    //     bool enabled = false;
        
	// 	QList<creative_kernel::ModelN*> models = selectionms();
	// 	for (creative_kernel::ModelN* model : models)
	// 	{
	// 		if (model->needRepair())
	// 		{
	// 			enabled = true;
    //             break;
	// 		}
	// 	}

    //     return enabled;
        return true;   
    }

    void RepairModelAction::onThemeChanged(ThemeCategory category)
    {
    }

    void RepairModelAction::onLanguageChanged(MultiLanguage language)
    {
        m_actionname = tr("Repair Model");
    }

    void RepairModelAction::slotRepairSuccess()
    {
        requestQmlDialog(this, "messageSingleBtnDlg");
    }
}