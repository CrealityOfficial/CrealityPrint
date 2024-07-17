#include "temperaturecommand.h"
#include "kernel/kernelui.h"
#include "kernel/kernel.h"
#include "interface/appsettinginterface.h"
#include "interface/commandinterface.h"
#include "interface/uiinterface.h"
#include "cxkernel/interface/jobsinterface.h"



namespace creative_kernel
{
    TemperatureCommand::TemperatureCommand(QObject* parent)
        :ActionCommand(parent)
    {
        m_actionname = tr("Temperature");
        m_actionNameWithout = "Temperature";
        m_eParentMenu = eMenuType_Calibration;

        addUIVisualTracer(this,this);
    }

    TemperatureCommand::~TemperatureCommand()
    {
    }

    void TemperatureCommand::execute()
    {
        requestQmlDialog(this, "TemperatureObj");
    }

	void TemperatureCommand::generate(int _statrt, int _end, int _step)
	{
		//Calibratejob* job = new Calibratejob(this);
		//m_creator.setType(CalibrateType::ct_temperature);
		//m_creator.setValue(_statrt, _end, _step);
		//job->setSceneCreator(&m_creator);
		//cxkernel::executeJob(qtuser_core::JobPtr(job));
		//getKernel()->sliceFlow()->calibrate(_statrt, _end, _step);
	}

	void TemperatureCommand::onThemeChanged(ThemeCategory category)
    {

    }

    void TemperatureCommand::onLanguageChanged(MultiLanguage language)
    {
        m_actionname = tr("Temperature");
    }
}
