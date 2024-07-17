#include "clonecommand.h"

#include "data/modeln.h"

#include "interface/selectorinterface.h"
#include "interface/commandinterface.h"
#include "interface/visualsceneinterface.h"
#include "cxkernel/interface/uiinterface.h"
#include "interface/machineinterface.h"

#include <QtCore/QDebug>
#include "job/nest2djob.h"

#include "kernel/kernelui.h"

namespace creative_kernel
{
    CloneCommand::CloneCommand(QObject* parent)
        :ToolCommand(parent)
    {
        orderindex = 3;
        m_name = tr("Clone") + ": Alt + C";

        m_source = "qrc:/kernel/qml/Clone.qml";
        addUIVisualTracer(this,this);
    }

    CloneCommand::~CloneCommand()
    {
    }

    void CloneCommand::onThemeChanged(ThemeCategory category)
    {
        setDisabledIcon("qrc:/UI/photo/cToolBar/clone_dark_disable.svg");
		setEnabledIcon(category == ThemeCategory::tc_dark ? "qrc:/UI/photo/cToolBar/clone_dark_default.svg" : "qrc:/UI/photo/cToolBar/clone_light_default.svg");
		setHoveredIcon(category == ThemeCategory::tc_dark ? "qrc:/UI/photo/cToolBar/clone_dark_default.svg" : "qrc:/UI/photo/cToolBar/clone_light_default.svg");
		setPressedIcon("qrc:/UI/photo/cToolBar/clone_dark_press.svg");
    }

    void CloneCommand::onLanguageChanged(MultiLanguage language)
    {
        m_name = tr("Clone") + ": Alt + C";
    }

    bool CloneCommand::isSelect()
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

    void CloneCommand::clone(int numb)
    {
        if (currentMachineIsBelt())
        {
            cmdClone(numb);
            return;
        }
        else
        {
            try {
                cmdClone(numb);
            }
            catch (...)
            {
                qDebug() << "CloneCommand::clone, catch(...)";
                return;
            }

            //Nest2DJob* job = new Nest2DJob(this);
            //QList<qtuser_core::JobPtr> jobs;
            //jobs.push_back(qtuser_core::JobPtr(job));
            //executeJobs(jobs);
        }
    }

    void CloneCommand::execute()
    {
        if (!m_pickOp)
        {
            m_pickOp = new PickOp(this);
            m_pickOp->setType(qtuser_3d::SceneOperateMode::FixedMode);
        }
        setVisOperationMode(m_pickOp);
        sensorAnlyticsTrace("Model Editing & Layout", "Clone");
    }

    bool CloneCommand::checkSelect()
    {
        QList<ModelN*> selections = selectionms();
        if (selections.size() > 0)
        {
            return true;
        }
        return false;
    }
}
