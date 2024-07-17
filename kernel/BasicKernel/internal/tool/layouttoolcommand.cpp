#include "layouttoolcommand.h"

#include "data/modeln.h"

#include "interface/selectorinterface.h"
#include "interface/commandinterface.h"
#include "cxkernel/interface/jobsinterface.h"
#include "cxkernel/wrapper/nest2djobex.h"

#include "utils/modelpositioninitializer.h"
#include "job/nest2djobex.h"
#include "job/extendfilljob.h"

#include <QtCore/QDebug>
#include "interface/visualsceneinterface.h"
#include "interface/machineinterface.h"

#include "kernel/kernelui.h"
#include "interface/uiinterface.h"
#include "interface/spaceinterface.h"
#include "interface/renderinterface.h"
#include "interface/printerinterface.h"
#include "interface/undoredointerface.h"
#include "internal/multi_printer/printer.h"
#include <external/kernel/kernel.h>
#include "internal/parameter/parametermanager.h"
#include "qtuser3d/trimesh2/conv.h"
#include <QTimer>

namespace creative_kernel
{
    LayoutToolCommand::LayoutToolCommand(QObject* parent)
        :ToolCommand(parent)
        , m_angle(0)
        , m_modelSpacing(1.0f)
        , m_platfromSpacing(2.0f)
        , m_bAlignMove(false)
        , m_checkPrinterLock(false)
        , m_hasDoneExtraLayout(false)
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

        qRegisterMetaType<LayoutChangeInfo>("LayoutChangeInfo");
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
            m_pickOp->setType(qtuser_3d::SceneOperateMode::FixedMode);
        }
        setVisOperationMode(m_pickOp);
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

	//void LayoutToolCommand::layoutByType(int type)
 //   {
 //       // emptyModels means layout all models in scene
 //       QList<ModelN*> emptyModels;

 //       m_checkPrinterLock = true;

 //       executeJobP(emptyModels, 0);
 //   }

    void LayoutToolCommand::autoLayout(int nlayoutWay)
    {
#ifdef DEBUG
        qDebug() << "m_layoutType =" << m_layoutType;
        qDebug() << "m_modelSpacing =" << m_modelSpacing;
        qDebug() << "m_platfromSpacing =" << m_platfromSpacing;
        qDebug() << "m_angle =" << m_angle;
#endif // DEBUG

        // layout all models in scene
        m_checkPrinterLock = true;
        QList<ModelN*> emptyModels;
        emptyModels.clear();
        m_waytype = (LayoutWay)nlayoutWay;

        int priorBinIndex = 0;
        bool needPrintersLayout = true;
        if (1 == m_layoutType)  // layout only selected models
        {
            priorBinIndex = currentPrinterIndex();
            needPrintersLayout = false;
        }     

        executeJobP(emptyModels, priorBinIndex, needPrintersLayout);

    }

    void LayoutToolCommand::layoutModels(QList<ModelN*> models, int priorBinIndex, bool needPrintersLayout, bool needAddToScene)
    {
        if (models.size() <= 0)
            return;

#ifdef DEBUG
        qDebug() << Q_FUNC_INFO;
        qDebug() << "m_layoutType =" << m_layoutType;
        qDebug() << "m_modelSpacing =" << m_modelSpacing;
        qDebug() << "m_platfromSpacing =" << m_platfromSpacing;
        qDebug() << "m_angle =" << m_angle;
#endif // DEBUG

        m_checkPrinterLock = true;

        if (!needAddToScene)
        {
            Printer *curPlatform = getSelectedPrinter();
            QString print_sequence =  curPlatform->parameter("print_sequence");
            QString gloab_sequence =  getKernel()->parameterManager()->printSequence();
            
            float extruder_clearance_radius = getKernel()->parameterManager()->extruder_clearance_radius();
            float distance = extruder_clearance_radius + 5;
            if (print_sequence.isEmpty())
            {
                print_sequence = gloab_sequence;
            }
            if (print_sequence == "by object")
            {
                m_modelSpacing = distance;
            }
            else
            {
                m_modelSpacing = 1;
            }
        }
        executeJobP(models, priorBinIndex, needPrintersLayout, needAddToScene);
    }

    void LayoutToolCommand::extendFillModel(ModelN* extendModel, int curBinIndex)
    {
        qDebug() << "extendFillModel = " << curBinIndex;
        executeExtendFillJob(extendModel, curBinIndex);
    }

    bool LayoutToolCommand::checkPrinterLock()
    {
        return m_checkPrinterLock;
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

    /*
    * call layout algorithm three times: 
    * (1) retrive the estimate printer size;
    * (2) move models , resize printer 
    * (3) possibly wipeTower involves in layout algorithm
    */
    void LayoutToolCommand::executeJobP(QList<ModelN*> models, int priorBinIndex, bool needPrintersLayout, bool needAddToScene)
    {
        // save the layout info for a second time layout algorithm call
        m_extraLayoutInfo.needLayoutModels = models;
        m_extraLayoutInfo.priorBinIndex = priorBinIndex;
        m_extraLayoutInfo.needPrintersLayout = needPrintersLayout;
        m_extraLayoutInfo.needAddToScene = needAddToScene;
        m_extraLayoutInfo.modelSpacing = m_modelSpacing;

        // set renderPolicy to always to prevent unexpected texture display when import models
        setContinousRender();

        m_checkPrinterLock = true;

        creative_kernel::Nest2DJobEx* job = new creative_kernel::Nest2DJobEx();

        job->setNestMode(int(cxkernel::NestMode::NEST_LAYOUT));
        job->setPriorBinIndex(priorBinIndex);
        job->setPrintersNeedLayout(needPrintersLayout);
        job->setNeedAddToScene(needAddToScene);

        if (models.size() > 0)  // means import model or copy model
        {
            job->setActiveLayoutModels(models);
        }

        job->setSelectAllFlag(!m_layoutType);

        job->setLayoutParameter(m_modelSpacing, m_platfromSpacing, m_angle, false, false); //  NOT use concave_outline in fdm; m_bAlignMove   To Do; 

        QObject::connect(job, &creative_kernel::Nest2DJobEx::extraLayout, this, &LayoutToolCommand::extraLayoutWork);

        cxkernel::executeJob(job);
    }

    void LayoutToolCommand::executeExtendFillJob(ModelN* extendModel, int curBinIndex)
    {
        Q_ASSERT(extendModel);

        // set renderPolicy to always to prevent unexpected texture display when import models
        setContinousRender();

        creative_kernel::ExtendFillJob* job = new creative_kernel::ExtendFillJob();
        job->setNestMode(int(cxkernel::NestMode::NEST_EXTEND_FILL));
        job->setExtendModel(extendModel);
        job->setExtendBinIndex(curBinIndex);

        job->setLayoutParameter(m_modelSpacing, m_platfromSpacing, m_angle, false, false);

        QObject::connect(job, &creative_kernel::ExtendFillJob::jobSuccessed, [this]() {
            QTimer::singleShot(1000, this, &LayoutToolCommand::releaseAlwaysRender);
            });

        cxkernel::executeJob(job);
    }

    void LayoutToolCommand::releaseAlwaysRender()
    {
        setCommandRender();
        //creative_kernel::requestVisUpdate(true);
    }

    void LayoutToolCommand::doExtraLayout(int estimatePrinterSize, bool finalFlag)
    {
        // set renderPolicy to always to prevent unexpected texture display when import models
        setContinousRender();

        creative_kernel::Nest2DJobEx* job = new creative_kernel::Nest2DJobEx();

        job->setNestMode(int(cxkernel::NestMode::NEST_LAYOUT));
        job->setPriorBinIndex(m_extraLayoutInfo.priorBinIndex);
        job->setPrintersNeedLayout(m_extraLayoutInfo.needPrintersLayout);
        job->setNeedAddToScene(m_extraLayoutInfo.needAddToScene);
        job->setActiveLayoutModels(m_extraLayoutInfo.needLayoutModels);
        job->setDoExtraFlag(true, finalFlag, estimatePrinterSize);

        if (finalFlag)
        {
            job->setSavedSceneInfo(m_currentSavedInfo);
        }

        job->setSelectAllFlag(!m_layoutType);

        job->setLayoutParameter(m_extraLayoutInfo.modelSpacing, m_platfromSpacing, m_angle, false, false); //  NOT use concave_outline in fdm; m_bAlignMove   To Do; 

        if (!finalFlag)
        {
            QObject::connect(job, &creative_kernel::Nest2DJobEx::secondLayoutSuccessed, this, &LayoutToolCommand::doFinalLayout);
        }
        else
        {
            QObject::connect(job, &creative_kernel::Nest2DJobEx::finalLayoutSuccessed, this, &LayoutToolCommand::onFinalLayoutSuccessed);
        }
        

        cxkernel::executeJob(job);
    }

    void LayoutToolCommand::extraLayoutWork(int estimatePrinterSize)
    {
        m_hasDoneExtraLayout = false;

        doExtraLayout(estimatePrinterSize, false);
    }

    void LayoutToolCommand::doFinalLayout(const LayoutChangeInfo& changeInfo)
    {
        setCommandRender();

        m_currentSavedInfo = changeInfo;

        if (!m_hasDoneExtraLayout)
        {
            m_hasDoneExtraLayout = true;
            doExtraLayout(printersCount(), true);
        }
    }

    void LayoutToolCommand::onFinalLayoutSuccessed()
    {
        setCommandRender();
    }

}
