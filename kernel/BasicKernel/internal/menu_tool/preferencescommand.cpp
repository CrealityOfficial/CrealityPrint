#include "preferencescommand.h"
#include "kernel/kernelui.h"
#include "kernel/kernel.h"
#include "interface/appsettinginterface.h"
#include "interface/commandinterface.h"
#include "interface/uiinterface.h"
#include "cxkernel/interface/jobsinterface.h"



namespace creative_kernel
{
    PreferencesCommand::PreferencesCommand(QObject* parent)
        :ActionCommand(parent)
    {
        m_actionname = tr("Preferences");
        m_actionNameWithout = "Preferences";
        m_eParentMenu = eMenuType_Tool;

        addUIVisualTracer(this,this);
    }
    PreferencesCommand::~PreferencesCommand()
    {
    }

    void PreferencesCommand::execute()
    {
        requestQmlDialog(this, "PreferencesObj");
    }

	void PreferencesCommand::generate(int _statrt, int _end, int _step)
	{
		//Calibratejob* job = new Calibratejob(this);
		//m_creator.setType(CalibrateType::ct_temperature);
		//m_creator.setValue(_statrt, _end, _step);
		//job->setSceneCreator(&m_creator);
		//cxkernel::executeJob(qtuser_core::JobPtr(job));
		//getKernel()->sliceFlow()->calibrate(_statrt, _end, _step);
	}

	void PreferencesCommand::onThemeChanged(ThemeCategory category)
    {

    }

    void PreferencesCommand::onLanguageChanged(MultiLanguage language)
    {
        m_actionname = tr("Preferences");
    }
}
