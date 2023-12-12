#include "layouttoolcommand.h"

#include "data/modeln.h"

#include "interface/selectorinterface.h"
#include "interface/commandinterface.h"
#include "cxkernel/interface/jobsinterface.h"

#include "utils/modelpositioninitializer.h"
#include "job/nest2djob.h"

#include <QtCore/QDebug>
#include "interface/visualsceneinterface.h"
#include "interface/machineinterface.h"

#include "kernel/kernelui.h"
#include "interface/uiinterface.h"

namespace creative_kernel
{
    LayoutToolCommand::LayoutToolCommand(QObject* parent)
        :ToolCommand(parent)
    {
        orderindex = 5;
        m_name = tr("Layout") + ": L";
        m_source = "qrc:/CrealityUI/qml/LayoutPanel.qml";
        addUIVisualTracer(this);
    }

    LayoutToolCommand::~LayoutToolCommand()
    {
        if (m_pickOp != nullptr)
        {
            delete m_pickOp;
            m_pickOp = nullptr;
        }
    }

    void LayoutToolCommand::execute()
    {
        if (!m_pickOp)
        {
            m_pickOp = new PickOp(this);
        }
        setVisOperationMode(m_pickOp);
    }

	bool LayoutToolCommand::isSelect()
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

    bool LayoutToolCommand::checkSelect()
    {
        QList<ModelN*> selections = selectionms();
        if (selections.size() > 0)
        {
            return true;
        }
        return false;
    }

	void LayoutToolCommand::layoutByType(int type) const
    {
        Nest2DJob* job = new Nest2DJob();
        cxkernel::NestPlaceType layoutType = cxkernel::NestPlaceType(type);

        if (!currentMachineIsBelt())
        {
            if (layoutType == cxkernel::NestPlaceType::CONCAVE_ALL)
                layoutType = cxkernel::NestPlaceType::CENTER_TO_SIDE;

            job->setNestType(layoutType);
        }
        else {
            job->setNestType(cxkernel::NestPlaceType::ONELINE);
        }

        QList<qtuser_core::JobPtr> jobs;
        jobs.push_back(qtuser_core::JobPtr(job));
        cxkernel::executeJobs(jobs);
    }

	void LayoutToolCommand::onThemeChanged(ThemeCategory category)
	{
        setDisabledIcon(category == ThemeCategory::tc_dark ? "qrc:/UI/photo/leftBar/layout_dark.svg" : "qrc:/UI/photo/leftBar/layout_lite.svg");
		setEnabledIcon(category == ThemeCategory::tc_dark ? "qrc:/UI/photo/leftBar/layout_dark.svg" : "qrc:/UI/photo/leftBar/layout_lite.svg");
		setHoveredIcon(category == ThemeCategory::tc_dark ? "qrc:/UI/photo/leftBar/layout_pressed.svg" : "qrc:/UI/photo/leftBar/layout_lite.svg");
		setPressedIcon(category == ThemeCategory::tc_dark ? "qrc:/UI/photo/leftBar/layout_pressed.svg" : "qrc:/UI/photo/leftBar/layout_pressed.svg");
	}

	void LayoutToolCommand::onLanguageChanged(MultiLanguage language)
	{
		m_name = tr("Layout") + ": L";
	}
}
