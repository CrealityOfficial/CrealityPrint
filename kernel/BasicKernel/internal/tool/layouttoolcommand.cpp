#include "layouttoolcommand.h"

#include "data/modeln.h"

#include "interface/selectorinterface.h"
#include "interface/commandinterface.h"
#include "cxkernel/interface/jobsinterface.h"
#include "cxkernel/wrapper/nest2djobex.h"

#include <QtCore/QDebug>
#include "interface/visualsceneinterface.h"
#include "interface/machineinterface.h"

#include "kernel/kernelui.h"
#include "interface/uiinterface.h"
#include "interface/spaceinterface.h"
#include "interface/renderinterface.h"
#include "interface/printerinterface.h"
#include "interface/layoutinterface.h"
#include "internal/multi_printer/printer.h"
#include <external/kernel/kernel.h>
#include "internal/parameter/parametermanager.h"
#include <QTimer>

namespace creative_kernel
{
    LayoutToolCommand::LayoutToolCommand(QObject* parent)
        :ToolCommand(parent)
        , m_angle(0)
        , m_modelSpacing(1.0f)
        , m_platfromSpacing(2.0f)
        , m_bAlignMove(false)
    {
        m_layoutType = 1;  //0: layout all models ; 1: layout selectmodel
        m_modelSpacing = 1;
        m_platfromSpacing = 5;
        //m_angle = 10;
        m_bAllowRotate = true;
        //m_angleList << 0 << 10 << 20 << 30 << 45 << 60 << 90 << 180;

        orderindex = 5;
        m_name = tr("Layout") + ": Shift+A";
        m_source = "qrc:/kernel/qml/LayoutPanel.qml";
        addUIVisualTracer(this,this);
    }

    LayoutToolCommand::~LayoutToolCommand()
    {
        //if (m_pickOp != nullptr)
        //{
        //    delete m_pickOp;
        //    m_pickOp = nullptr;
        //}
    }

    void LayoutToolCommand::execute()
    {
        //if (!m_pickOp)
        //{
        //    m_pickOp = new PickOp(this);
        //    m_pickOp->setType(qtuser_3d::SceneOperateMode::FixedMode);
        //}
        //setVisOperationMode(m_pickOp);
        creative_kernel::setVisOperationMode(nullptr);
        Q_EMIT typeChanged();
        Q_EMIT modelSpacingChanged();
        Q_EMIT platformSpacingChanged();
        Q_EMIT angleIndexChanged();
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

    float LayoutToolCommand::getModelSpacingByPrintSequence()
    {
        float spacingVal = 0.0f;

		Printer* curPlatform = getSelectedPrinter();
		QString print_sequence = curPlatform->parameter("print_sequence");
		QString gloab_sequence = getKernel()->parameterManager()->printSequence();

		float extruder_clearance_radius = getKernel()->parameterManager()->extruder_clearance_radius();
		float distance = extruder_clearance_radius + 5;
		if (print_sequence.isEmpty())
		{
			print_sequence = gloab_sequence;
		}
		if (print_sequence == "by object")
		{
            spacingVal = distance;
		}
		else
		{
            spacingVal = m_modelSpacing;
		}

        return spacingVal;
    }

    void LayoutToolCommand::autoLayout(int nlayoutWay)
    {
#ifdef DEBUG
        qDebug() << "m_layoutType =" << m_layoutType;
        qDebug() << "m_modelSpacing =" << m_modelSpacing;
        qDebug() << "m_platfromSpacing =" << m_platfromSpacing;
        qDebug() << "m_angle =" << m_angle;
#endif // DEBUG

        // layout all models in scene
        QList<int> emptyGroupIds;
        emptyGroupIds.clear();
        //m_waytype = (LayoutWay)nlayoutWay;

        int priorBinIndex = 0;
        if (1 == m_layoutType)  // layout only selected models
        {
            priorBinIndex = currentPrinterIndex();
        }

        layoutModelGroups(emptyGroupIds, priorBinIndex);

    }
	
    void LayoutToolCommand::onThemeChanged(ThemeCategory category)
	{
        setDisabledIcon("qrc:/UI/photo/cToolBar/layout_dark_disable.svg");
		setEnabledIcon(category == ThemeCategory::tc_dark ? "qrc:/UI/photo/cToolBar/layout_dark_default.svg": "qrc:/UI/photo/cToolBar/layout_light_default.svg");
		setHoveredIcon(category == ThemeCategory::tc_dark ? "qrc:/UI/photo/cToolBar/layout_dark_default.svg": "qrc:/UI/photo/cToolBar/layout_light_default.svg");
		setPressedIcon("qrc:/UI/photo/cToolBar/layout_dark_press.svg");
	}

	void LayoutToolCommand::onLanguageChanged(MultiLanguage language)
	{
		m_name = tr("Layout") + ": Shift+A";
	}

    float LayoutToolCommand::modelSpacing()
    {
        qtuser_core::VersionSettings setting;
        setting.beginGroup("profile_setting");
        float modelSpacing = setting.value("model_spacing", 1).toFloat();
        setting.endGroup();

        m_modelSpacing = modelSpacing;
        return m_modelSpacing;
    }

}
