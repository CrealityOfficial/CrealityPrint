#include "layoutmanager.h"

#include "nest2djobex.h"
#include "extendfilljob.h"
#include "simplelayout.h"

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
#include "interface/undoredointerface.h"
#include "internal/multi_printer/printer.h"
#include "external/kernel/kernel.h"
#include "internal/parameter/parametermanager.h"
#include "internal/tool/layouttoolcommand.h"
#include <QTimer>
#include "qtuser3d/trimesh2/conv.h"
#include "layoutitemex.h"

namespace creative_kernel
{
    LayoutManager::LayoutManager(QObject* parent)
        :QObject(parent)
    {
    }

    LayoutManager::~LayoutManager()
    {
    }

    void LayoutManager::getPrinterFixItemInfo(int printerIdx, std::vector<LayoutNestFixedItemInfo>& outFixInfos)
    {
        Printer* aPrinter = getPrinter(printerIdx);
        if (!aPrinter)
            return;

        outFixInfos.clear();

        creative_kernel::LayoutNestFixedItemInfo tmpFixItemInfo;

        // only one printer
        tmpFixItemInfo.fixBinId = 0;

        QList<ModelGroup*> inPrinterGroups = getPrinterModelGroups(printerIdx);

        // fixed group info
        for (ModelGroup* aGroup : inPrinterGroups)
        {
            creative_kernel::LayoutItemEx fixitem(aGroup->getObjectId(), false);

            //fixs use global outline
            tmpFixItemInfo.fixOutline = fixitem.outLine();
            tmpFixItemInfo.fixBinGlobalBox = qtuser_3d::qBox32box3(aPrinter->globalBox());

            outFixInfos.push_back(tmpFixItemInfo);
        }

        // fixed wipe tower info
        if (!aPrinter->checkWipeTowerShouldShow())
            return;

        tmpFixItemInfo.fixOutline = aPrinter->wipeTowerOutlinePath(true);
        tmpFixItemInfo.fixBinGlobalBox = qtuser_3d::qBox32box3(aPrinter->globalBox());

        outFixInfos.push_back(tmpFixItemInfo);
    }

    void LayoutManager::layoutModelGroups(QList<int> groupIds, int priorBinIndex)
    {
        executeJobP(groupIds, priorBinIndex);
    }

    void LayoutManager::extendFillModelGroup(int groupId, int curBinIndex)
    {
        if (curBinIndex < 0 || curBinIndex >= printersCount())
            return;

        qDebug() << "extendFillModel = " << curBinIndex;
        executeExtendFillJob(groupId, curBinIndex);
    }

    void LayoutManager::getPositionBySimpleLayout(const QList<CONTOUR_PATH>& inOutlinePaths, QList<QVector3D>& outPos)
    {
        SimpleLayout simLayout;

        LayoutParameterInfo  simpleLayoutParameterInfo;
        simpleLayoutParameterInfo.modelSpacing = getKernelUI()->getLayoutCommand()->modelSpacing();
        simpleLayoutParameterInfo.platfromSpacing = getKernelUI()->getLayoutCommand()->platformSpacing();
        simpleLayoutParameterInfo.priorBinIndex = currentPrinter()->index();

        simLayout.setLayoutParameterInfo(simpleLayoutParameterInfo);

        std::vector<LayoutNestFixedItemInfo> printerFixInfos;
        getPrinterFixItemInfo(currentPrinter()->index(), printerFixInfos);
        simLayout.setFixItemInfo(printerFixInfos);

        simLayout.setActiveItemInfo(inOutlinePaths);

        simLayout.doLayout();

        outPos = simLayout.getLayoutPos();
    }

    /*
    * call layout algorithm two times:
    * (1) retrive the estimate printer size;
    * (2) move models , resize printer
    */
    void LayoutManager::executeJobP(QList<int> groupIds, int priorBinIndex)
    {
        // save the layout info for a second time layout algorithm call
        m_extraLayoutParameterInfo.needLayoutGroupIds = groupIds;
        m_extraLayoutParameterInfo.priorBinIndex = priorBinIndex;
        m_extraLayoutParameterInfo.modelSpacing = getKernelUI()->getLayoutCommand()->getModelSpacingByPrintSequence();
        m_extraLayoutParameterInfo.platfromSpacing = getKernelUI()->getLayoutCommand()->platformSpacing();
        m_extraLayoutParameterInfo.angle = getKernelUI()->getLayoutCommand()->angle();
        m_extraLayoutParameterInfo.layoutType = getKernelUI()->getLayoutCommand()->type();
        m_extraLayoutParameterInfo.alignMove = false;
        m_extraLayoutParameterInfo.outlineConcave = false;

        // set renderPolicy to always to prevent unexpected texture display when import models
        requestRender(this);

        creative_kernel::Nest2DJobEx* job = new creative_kernel::Nest2DJobEx();
        job->setLayoutParameterInfo(m_extraLayoutParameterInfo);
        QObject::connect(job, &creative_kernel::Nest2DJobEx::extraLayout, this, &LayoutManager::doExtraLayout);
        QObject::connect(job, &creative_kernel::Nest2DJobEx::layoutSuccessed, this, &LayoutManager::onLayoutSuccessed);
        cxkernel::executeJob(job);
    }

    void LayoutManager::executeExtendFillJob(int extendGroupId, int curBinIndex)
    {
        LayoutParameterInfo  paramInfo;
        paramInfo.needLayoutGroupIds.push_back(extendGroupId);
        paramInfo.priorBinIndex = curBinIndex;
        paramInfo.modelSpacing = getKernelUI()->getLayoutCommand()->modelSpacing();
        paramInfo.platfromSpacing = getKernelUI()->getLayoutCommand()->platformSpacing();
        paramInfo.angle = getKernelUI()->getLayoutCommand()->angle();
        paramInfo.layoutType = getKernelUI()->getLayoutCommand()->type();
        paramInfo.alignMove = false;
        paramInfo.outlineConcave = false;

        // set renderPolicy to always to prevent unexpected texture display when import models
        requestRender(this);

        creative_kernel::ExtendFillJob* job = new creative_kernel::ExtendFillJob();
        job->setLayoutParamInfo(paramInfo);

        QObject::connect(job, &creative_kernel::ExtendFillJob::jobSuccessed, [this]() {
            QTimer::singleShot(1000, this, &LayoutManager::releaseAlwaysRender);
            });

        cxkernel::executeJob(job);
    }

    void LayoutManager::releaseAlwaysRender()
    {
        endRequestRender(this);
        //creative_kernel::requestVisUpdate(true);
    }

    void LayoutManager::doExtraLayout(int estimatePrinterSize)
    {
        // set renderPolicy to always to prevent unexpected texture display when import models
        requestRender(this);

        creative_kernel::Nest2DJobEx* job = new creative_kernel::Nest2DJobEx();
        job->setLayoutParameterInfo(m_extraLayoutParameterInfo);
        job->setDoExtraFlag(true, estimatePrinterSize);

        QObject::connect(job, &creative_kernel::Nest2DJobEx::layoutSuccessed, this, &LayoutManager::onLayoutSuccessed);

        cxkernel::executeJob(job);
    }

    void LayoutManager::onLayoutSuccessed(const LayoutChangeInfo& changeInfo)
    {
        endRequestRender(this);

        layoutChangeScene(changeInfo);

    }

}
